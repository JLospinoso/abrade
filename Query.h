#pragma once

#include <boost/asio/spawn.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <iostream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include "Action.h"
#include "Exception.h"
#include "Candidate.h"
#include "HttpStatus.h"
#include "NetworkTimeout.h"

//TODO: PostQuery

/// Executes GET requests and forwards successful response bodies to a GetAction.
struct GetQuery {
  GetQuery(GetAction response_action, bool should_print_found, bool verbose_output) : print_found{should_print_found}, verbose{verbose_output},
                                                                                     action{std::move(response_action)} {}

  /// Reads the full response, processes it, and prints status according to output options.
  template <typename Stream>
  void execute(Stream& stream, const std::string_view& description, const boost::asio::yield_context& yield) {
    boost::beast::flat_buffer buffer;
    boost::beast::http::response<boost::beast::http::dynamic_body> response;
    await_stream_with_timeout(stream, "get query", yield, [&stream, &buffer, &response](auto token) {
      boost::beast::http::async_read(stream, buffer, response, token);
    });
    action.process(response.result_int(), response, description);

    const auto status_code = response.result_int();
    if (print_found && is_success_status(status_code)) {
      std::cout << "[+] Status of " << description << ": " << status_code << std::endl;
    }
    else if (verbose) { std::cout << "[-] Status of " << description << ": " << status_code << std::endl; }
  }

  template <typename Request>
  static void set_method(Request& request) { request.method(boost::beast::http::verb::get); }

private:
  bool print_found, verbose;
  GetAction action;
};

/// Executes HEAD requests and forwards successful candidates to a HeadAction.
struct HeadQuery {
  HeadQuery(HeadAction response_action, bool should_print_found, bool verbose_output) : print_found{should_print_found}, verbose{verbose_output},
                                                                                       action {std::move(response_action)} { }

  /// Reads response headers, processes status, and prints status according to output options.
  template <typename Stream>
  void execute(Stream& stream, const std::string_view& description, const boost::asio::yield_context& yield) {
    boost::beast::flat_buffer buffer;
    boost::beast::http::response_parser<boost::beast::http::empty_body> parser;
    await_stream_with_timeout(stream, "head query", yield, [&stream, &buffer, &parser](auto token) {
      boost::beast::http::async_read_header(stream, buffer, parser, token);
    });
    const auto& response = parser.release();
    action.process(response.result_int(), description);

    const auto status_code = response.result_int();
    if (print_found && is_success_status(status_code)) {
      std::cout << "[+] Status of " << description << ": " << status_code << std::endl;
    }
    else if (verbose) { std::cout << "[-] Status of " << description << ": " << status_code << std::endl; }
  }

  template <typename Request>
  static void set_method(Request& request) { request.method(boost::beast::http::verb::head); }

private:
  bool print_found, verbose;
  HeadAction action;
};
