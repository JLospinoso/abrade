#pragma once
#include <abrade/candidate.hpp>
#include <abrade/controller.hpp>
#include <abrade/run_stats.hpp>
#include <abrade/scraper_runtime.hpp>
#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http/parser.hpp>
#include <fstream>
#include <iostream>
#include <ostream>
#include <string>

namespace abrade {

/// Coordinates generation, connection setup, request writing, query execution, and error logging.
///
/// `Scraper` is deliberately templated over the runtime policies it coordinates.
/// That keeps transport, request construction, query behavior, output, and error
/// logging testable without making Abrade an installed C++ library.
template <typename Generator, typename Query, typename Connection, typename RequestWriter,
          typename ErrorLog>
struct Scraper {
  Scraper(Query&& query_in, Connection&& connection_in, RequestWriter&& writer_in,
          ErrorLog&& error_log_in, Controller& controller_in, boost::asio::io_context& io_context,
          RunStats& run_stats)
      : error_log{std::forward<ErrorLog>(error_log_in)}, controller{controller_in}, ios{io_context},
        query{std::forward<Query>(query_in)}, connection{std::forward<Connection>(connection_in)},
        writer{std::forward<RequestWriter>(writer_in)}, stats{run_stats}, active_coroutines{} {}

  /// Starts the coroutine pool and runs the associated io_context to completion.
  void run(Generator& generator) {
    spawn_coroutine(generator);
    ios.run();
  }

private:
  struct LifetimeCounter {
    /// Increments the shared active-coroutine count for this coroutine lifetime.
    explicit LifetimeCounter(size_t& counter) : value{counter} { value++; }
    LifetimeCounter(const LifetimeCounter&) = delete;
    LifetimeCounter(LifetimeCounter&&) = delete;
    LifetimeCounter& operator=(const LifetimeCounter&) = delete;
    LifetimeCounter& operator=(LifetimeCounter&&) = delete;
    ~LifetimeCounter() { value--; }

  private:
    size_t& value;
  };

  void spawn_coroutine(Generator& generator) {
    spawn(
        ios,
        [this, &generator](const boost::asio::yield_context& yield) {
          LifetimeCounter ctr{active_coroutines};
          while (const auto uri = generator.next()) {
            if (active_coroutines < controller.recommended_coroutines()) {
              spawn_coroutine(generator);
            }
            // Candidate enrichment belongs in make_candidate so Scraper stays an orchestrator.
            auto candidate = make_candidate(*uri);
            try {
              coroutine(yield, candidate);
            } catch (const std::exception& e) {
              stats.record_error();
              error_log.record(*uri, e);
            }
            controller.register_completion(active_coroutines);
            if (active_coroutines > controller.recommended_coroutines()) {
              return;
            }
          }
        },
        boost::asio::detached);
  }

  void coroutine(const boost::asio::yield_context& yield, const Candidate& candidate) {
    auto current = candidate;
    std::size_t redirects_followed{};
    while (true) {
      boost::asio::ip::tcp::socket sock{ios};
      stats.record_attempt();
      auto instance = connection.connect(sock, yield);
      writer.make_request(instance->get(), query, current, yield);
      const auto redirect = query.execute(instance->get(), current.description(), yield);
      if (!redirect || redirects_followed == query.max_redirects()) {
        return;
      }
      current.uri = *redirect;
      redirects_followed++;
    }
  }

  ErrorLog error_log;
  Controller& controller;
  boost::asio::io_context& ios;
  Query query;
  Connection connection;
  RequestWriter writer;
  RunStats& stats;
  size_t active_coroutines;
};
} // namespace abrade
