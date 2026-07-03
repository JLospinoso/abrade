#pragma once

#include <boost/asio/spawn.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/beast/core.hpp>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include "Exception.h"

/// Returns the per-operation network timeout, optionally overridden by ABRADE_NETWORK_TIMEOUT_MS.
inline std::chrono::milliseconds network_operation_timeout() {
  static const auto timeout = [] {
    constexpr auto default_timeout = std::chrono::milliseconds{30000};
    const auto* raw_timeout = std::getenv("ABRADE_NETWORK_TIMEOUT_MS");
    if (raw_timeout == nullptr || raw_timeout[0] == '\0') {
      return default_timeout;
    }

    char* end{};
    const auto parsed = std::strtoull(raw_timeout, &end, 10);
    if (*end != '\0' || parsed == 0 || parsed > static_cast<unsigned long long>(std::numeric_limits<int>::max())) {
      return default_timeout;
    }

    return std::chrono::milliseconds{parsed};
  }();

  return timeout;
}

/// Awaits a void asynchronous operation and cancels it if the network timeout expires.
template <typename Executor, typename Cancel, typename Start>
void await_void_with_timeout(Executor executor,
                             Cancel&& cancel,
                             std::string_view action,
                             const boost::asio::yield_context& yield,
                             Start&& start) {
  auto active = std::make_shared<std::atomic_bool>(true);
  auto timed_out = std::make_shared<std::atomic_bool>(false);
  auto cancel_operation = std::make_shared<std::decay_t<Cancel>>(std::forward<Cancel>(cancel));
  boost::asio::steady_timer timer{executor};
  timer.expires_after(network_operation_timeout());
  timer.async_wait([active, timed_out, cancel_operation](const boost::system::error_code& ec) {
    if (!ec && active->exchange(false)) {
      timed_out->store(true);
      (*cancel_operation)();
    }
  });

  boost::system::error_code ec;
  std::forward<Start>(start)(yield[ec]);
  active->store(false);
  timer.cancel();

  if (timed_out->load()) {
    throw AbradeException{std::string{action} + " timeout"};
  }
  if (ec) {
    throw AbradeException{std::string{action}, ec};
  }
}

/// Awaits an asynchronous operation with a result and cancels it if the network timeout expires.
template <typename Result, typename Executor, typename Cancel, typename Start>
Result await_result_with_timeout(Executor executor,
                                 Cancel&& cancel,
                                 std::string_view action,
                                 const boost::asio::yield_context& yield,
                                 Start&& start) {
  auto active = std::make_shared<std::atomic_bool>(true);
  auto timed_out = std::make_shared<std::atomic_bool>(false);
  auto cancel_operation = std::make_shared<std::decay_t<Cancel>>(std::forward<Cancel>(cancel));
  boost::asio::steady_timer timer{executor};
  timer.expires_after(network_operation_timeout());
  timer.async_wait([active, timed_out, cancel_operation](const boost::system::error_code& ec) {
    if (!ec && active->exchange(false)) {
      timed_out->store(true);
      (*cancel_operation)();
    }
  });

  boost::system::error_code ec;
  auto result = std::forward<Start>(start)(yield[ec]);
  active->store(false);
  timer.cancel();

  if (timed_out->load()) {
    throw AbradeException{std::string{action} + " timeout"};
  }
  if (ec) {
    throw AbradeException{std::string{action}, ec};
  }

  return result;
}

/// Applies the standard timeout wrapper to stream-based asynchronous operations.
template <typename Stream, typename Start>
void await_stream_with_timeout(Stream& stream,
                               std::string_view action,
                               const boost::asio::yield_context& yield,
                               Start&& start) {
  auto& lowest_layer = boost::beast::get_lowest_layer(stream);
  await_void_with_timeout(
    lowest_layer.get_executor(),
    [&lowest_layer] {
      boost::system::error_code ignored;
      lowest_layer.cancel(ignored);
    },
    action,
    yield,
    std::forward<Start>(start)
  );
}

/// Applies the standard timeout wrapper to resolver asynchronous operations.
template <typename Result, typename Resolver, typename Start>
Result await_resolver_with_timeout(Resolver& resolver,
                                   std::string_view action,
                                   const boost::asio::yield_context& yield,
                                   Start&& start) {
  return await_result_with_timeout<Result>(
    resolver.get_executor(),
    [&resolver] { resolver.cancel(); },
    action,
    yield,
    std::forward<Start>(start)
  );
}
