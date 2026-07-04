#pragma once
#include <abrade/candidate.hpp>
#include <abrade/exception.hpp>
#include <abrade/network_timeout.hpp>
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <string>

namespace abrade {

using RequestType = boost::beast::http::request<boost::beast::http::string_body>;

/// Builds the immutable pieces shared by every generated HTTP request.
///
/// The query layer supplies the method and the generator supplies the target.
/// Host, User-Agent, and HTTP version are stable for one CLI invocation.
inline RequestType build_request_template(const std::string& host_name,
                                          const std::string& user_agent) {
  RequestType request;
  request.set(boost::beast::http::field::host, host_name);
  request.set(boost::beast::http::field::user_agent,
              user_agent); // TODO: Might be able to fold into headers?
  request.version(11);
  return request;
}

/// Writes an HTTP request for a generated candidate to an established stream.
///
/// `RequestWriter` intentionally knows only about request construction. It does
/// not own connection setup, response parsing, or output side effects.
struct RequestWriter {
  RequestWriter(const std::string& host_name, bool verbose_output, const std::string& user_agent)
      : is_verbose{verbose_output},
        request_template{build_request_template(host_name, user_agent)} {}

  /// Applies the query method and candidate URI, then writes the request asynchronously.
  template <typename Stream, typename Query>
  void make_request(Stream&& stream, const Query& query, const Candidate& candidate,
                    const boost::asio::yield_context& yield) {
    auto request{request_template};
    query.set_method(request);
    request.target(candidate.uri);
    // TODO: Content, headers.
    request.prepare_payload();
    if (is_verbose) {
      std::cout << "[ ] Payload for " << candidate.uri << ": " << request;
    }
    await_stream_with_timeout(stream, "make request", yield, [&stream, &request](auto token) {
      boost::beast::http::async_write(stream, request, token);
    });
  }

private:
  const bool is_verbose;
  const RequestType request_template;
};
} // namespace abrade
