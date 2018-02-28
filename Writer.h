#pragma once
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <string>
#include "Exception.h"

using RequestType = boost::beast::http::request<boost::beast::http::string_body>;

inline RequestType build_request_template(const std::string& host_name, const std::string& user_agent) {
  RequestType request;
  request.set(boost::beast::http::field::host, host_name);
  request.set(boost::beast::http::field::user_agent, user_agent);
  request.version(11);
  return request;
}

struct RequestWriter {
  RequestWriter(const std::string& host_name, bool is_verbose, const std::string& user_agent)
    : is_verbose{ is_verbose }, request_template{ build_request_template(host_name, user_agent) } {}

  template <typename Stream, typename Query>
  void make_request(Stream&& stream, const Query& query, const std::string& target,
                    const boost::asio::yield_context& yield) {
    auto request{request_template};
    query.set_method(request);
    request.target(target);
    request.prepare_payload();
    if (is_verbose) std::cout << "[ ] Payload for " << target << ": " << request;
    boost::system::error_code ec;
    boost::beast::http::async_write(stream, request, yield[ec]);
    if (ec) throw AbradeException{"make request", ec};
  }

private:
  const bool is_verbose;
  const RequestType request_template;
};
