#pragma once

#include <boost/asio/spawn.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <string>
#include <system_error>
#include "Action.h"
#include "Exception.h"
#include "Candidate.h"
#include "HttpStatus.h"
#include "NetworkTimeout.h"

//TODO: PostQuery

struct GetQuery {
  GetQuery(GetAction action, bool print_found, bool verbose) : print_found{print_found}, verbose{verbose},
                                                               action{std::move(action)} {}

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
  void set_method(Request& request) const { request.method(boost::beast::http::verb::get); }

private:
  bool print_found, verbose;
  GetAction action;
};

struct HeadQuery {
  HeadQuery(HeadAction action, bool print_found, bool verbose) : print_found{print_found}, verbose{verbose},
                                                                 action {std::move(action)} { }

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
  void set_method(Request& request) const { request.method(boost::beast::http::verb::head); }

private:
  bool print_found, verbose;
  HeadAction action;
};
