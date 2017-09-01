#pragma once
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <string>
#include "Exception.h"

struct RequestWriter {
  RequestWriter(const std::string& host_name, bool is_verbose, const std::string& user_agent)
    : is_verbose{ is_verbose } {
    request.set(boost::beast::http::field::host, host_name);
    request.set(boost::beast::http::field::user_agent, user_agent);
    request.version = 11;
  }

  template <typename Stream, typename Query>
  void make_request(Stream&& stream, const Query& query, const std::string& target,
                    const boost::asio::yield_context& yield) {
    auto req{request};
    query.set_method(req);
    req.target(target);
    req.prepare_payload();
    if (is_verbose) std::cout << "[ ] Payload for " << target << ": " << req;
    boost::system::error_code ec;
    boost::beast::http::async_write(stream, req, yield[ec]);
    if (ec) throw AbradareException{"make request", ec};
  }

private:
  bool is_verbose;
  boost::beast::http::request<boost::beast::http::string_body> request;
};
