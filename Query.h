#pragma once

#include <boost/asio/spawn.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <string>
#include <system_error>
#include "Action.h"
#include "Exception.h"

struct GetQuery {
  GetQuery(GetAction action, bool print_found, bool verbose) : print_found{print_found}, verbose{verbose},
                                                               action{std::move(action)} {}

  template <typename Stream>
  void execute(Stream& stream, const std::string& target, const boost::asio::yield_context& yield) {
    boost::system::error_code ec;
    boost::beast::flat_buffer buffer;
    boost::beast::http::response<boost::beast::http::dynamic_body> response;
    boost::beast::http::async_read(stream, buffer, response, yield[ec]);
    if (ec) throw AbradareException{"get query", ec};
    action.process(response.result_int(), response, target);

    const auto status_code = response.result_int();
    if (print_found && (status_code >= 200 && status_code < 300)) {
      std::cout << "[+] Status of " << target << ": " << status_code << std::endl;
    }
    else if (verbose) { std::cout << "[-] Status of " << target << ": " << status_code << std::endl; }
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
  void execute(Stream& stream, const std::string& target, const boost::asio::yield_context& yield) {
    boost::system::error_code ec;
    boost::beast::flat_buffer buffer;
    boost::beast::http::response_parser<boost::beast::http::empty_body> parser;
    boost::beast::http::async_read_header(stream, buffer, parser, yield[ec]);
    if (ec) throw AbradareException{"head query", ec};
    const auto& response = parser.release();
    action.process(response.result_int(), target);

    const auto status_code = response.result_int();
    if (print_found && (status_code >= 200 && status_code < 300)) {
      std::cout << "[+] Status of " << target << ": " << status_code << std::endl;
    }
    else if (verbose) { std::cout << "[-] Status of " << target << ": " << status_code << std::endl; }
  }

  template <typename Request>
  void set_method(Request& request) const { request.method(boost::beast::http::verb::head); }

private:
  bool print_found, verbose;
  HeadAction action;
};
