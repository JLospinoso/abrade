#pragma once

#include <abrade/action.hpp>
#include <abrade/candidate.hpp>
#include <abrade/exception.hpp>
#include <abrade/http_status.hpp>
#include <abrade/network_timeout.hpp>
#include <abrade/redirect_policy.hpp>
#include <abrade/run_stats.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/http.hpp>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

namespace abrade {

// TODO: Add POST-style queries only after Candidate request bodies become product behavior.

/// Executes GET requests and forwards response bodies to a `GetAction`.
///
/// Status printing is owned here because the query layer sees both the response
/// status and the user-facing candidate description.
struct GetQuery {
  GetQuery(GetAction response_action, bool should_print_found, bool verbose_output,
           RedirectPolicy redirect_options, RunStats& run_stats)
      : print_found{should_print_found}, verbose{verbose_output},
        redirect_policy{std::move(redirect_options)}, stats{run_stats},
        action{std::move(response_action)} {}

  /// Reads the full response, processes it, prints status, and returns a follow-up redirect target.
  template <typename Stream>
  std::optional<std::string> execute(Stream& stream, const std::string_view& description,
                                     const boost::asio::yield_context& yield) {
    boost::beast::flat_buffer buffer;
    boost::beast::http::response<boost::beast::http::dynamic_body> response;
    await_stream_with_timeout(stream, "get query", yield,
                              [&stream, &buffer, &response](auto token) {
                                boost::beast::http::async_read(stream, buffer, response, token);
                              });
    const auto status_code = response.result_int();
    stats.record_response(status_code);
    const auto body = boost::beast::buffers_to_string(response.body().data());
    action.process(status_code, body, description);

    if (print_found && is_success_status(status_code)) {
      std::cout << "[+] Status of " << description << ": " << status_code << '\n';
    } else if (verbose) {
      std::cout << "[-] Status of " << description << ": " << status_code << '\n';
    }
    return redirect_policy.redirect_target(status_code, response.base(), description);
  }

  /// Returns the configured maximum number of redirect hops.
  [[nodiscard]] std::size_t max_redirects() const noexcept { return redirect_policy.max_redirects; }

  /// Applies the HTTP method expected by this query type.
  template <typename Request> static void set_method(Request& request) {
    request.method(boost::beast::http::verb::get);
  }

private:
  bool print_found, verbose;
  RedirectPolicy redirect_policy;
  RunStats& stats;
  GetAction action;
};

/// Executes HEAD requests and forwards candidate status to a `HeadAction`.
///
/// Only response headers are read. This is the default probe mode because it
/// avoids response-body transfer when existence is the only question.
struct HeadQuery {
  HeadQuery(HeadAction response_action, bool should_print_found, bool verbose_output,
            RedirectPolicy redirect_options, RunStats& run_stats)
      : print_found{should_print_found}, verbose{verbose_output},
        redirect_policy{std::move(redirect_options)}, stats{run_stats},
        action{std::move(response_action)} {}

  /// Reads response headers, processes status, prints status, and returns a follow-up redirect
  /// target.
  template <typename Stream>
  std::optional<std::string> execute(Stream& stream, const std::string_view& description,
                                     const boost::asio::yield_context& yield) {
    boost::beast::flat_buffer buffer;
    boost::beast::http::response_parser<boost::beast::http::empty_body> parser;
    await_stream_with_timeout(stream, "head query", yield, [&stream, &buffer, &parser](auto token) {
      boost::beast::http::async_read_header(stream, buffer, parser, token);
    });
    const auto& response = parser.release();
    const auto status_code = response.result_int();
    stats.record_response(status_code);
    action.process(response.result_int(), description);

    if (print_found && is_success_status(status_code)) {
      std::cout << "[+] Status of " << description << ": " << status_code << '\n';
    } else if (verbose) {
      std::cout << "[-] Status of " << description << ": " << status_code << '\n';
    }
    return redirect_policy.redirect_target(status_code, response.base(), description);
  }

  /// Returns the configured maximum number of redirect hops.
  [[nodiscard]] std::size_t max_redirects() const noexcept { return redirect_policy.max_redirects; }

  /// Applies the HTTP method expected by this query type.
  template <typename Request> static void set_method(Request& request) {
    request.method(boost::beast::http::verb::head);
  }

private:
  bool print_found, verbose;
  RedirectPolicy redirect_policy;
  RunStats& stats;
  HeadAction action;
};
} // namespace abrade
