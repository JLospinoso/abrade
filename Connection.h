#pragma once
#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/spawn.hpp>
#include <string>
#include <array>
#include "Exception.h"
#include "Endpoint.h"
#include "NetworkTimeout.h"
#include <utility>
#include <memory>
#include <vector>

template <typename Stream>
struct Connection {
  template <typename... Args>
  Connection(std::function<void(Stream&)>&& cleanup, Args&&... args) : stream {std::forward<Args>(args)...},
                                                                       cleanup{std::move(cleanup)} { }

  ~Connection() {
    try { if (cleanup) cleanup(stream); }
    catch (std::exception& e) { std::cerr << "[-] Connection caught exception: " << e.what() << std::endl; }
  }

  Connection(const Connection&) = delete;
  Connection& operator=(const Connection&) = delete;
  Connection(Connection&&) = delete;
  Connection& operator=(Connection&&) = delete;
  Stream& get() { return stream; }
private:
  Stream stream;
  std::function<void(Stream&)> cleanup;
};

struct PlaintextConnection {
  PlaintextConnection(const std::string& host_name, bool sensitive_teardown, boost::asio::io_context& ios)
    : sensitive_teardown{sensitive_teardown},
      endpoint{parse_host_endpoint(host_name, "80", 80)},
      ios{ios} { }

  auto connect(
    boost::asio::ip::tcp::socket& sock,
    const boost::asio::yield_context& yield) {
    auto result = std::make_unique<Connection<boost::asio::ip::tcp::socket&>>(
      [st = sensitive_teardown](auto& stream) {
      boost::system::error_code shutdown_error;
      stream.shutdown(boost::asio::ip::tcp::socket::shutdown_both, shutdown_error);
      if (st && shutdown_error != boost::asio::error::eof)
        throw AbradeException{
          "tcp shutdown", shutdown_error
        };
    },
      sock);

    boost::asio::ip::tcp::resolver resolver{ios};
    const auto lookup_result = await_resolver_with_timeout<boost::asio::ip::tcp::resolver::results_type>(
      resolver,
      "resolve host",
      yield,
      [this, &resolver](auto token) { return resolver.async_resolve(endpoint.host, endpoint.service, token); }
    );
    await_stream_with_timeout(result->get(), "tcp connect", yield, [&result, &lookup_result](auto token) {
      boost::asio::async_connect(result->get(), lookup_result, token);
    });

    return std::move(result);
  }

private:
  bool sensitive_teardown;
  HostEndpoint endpoint;
  boost::asio::io_context& ios;
};

struct TlsConnection {
  TlsConnection(const std::string& host_name, bool is_verify, bool sensitive_teardown, boost::asio::io_context& ios)
    : sensitive_teardown{sensitive_teardown},
      endpoint{parse_host_endpoint(host_name, "443", 443)},
      context{boost::asio::ssl::context::tls_client},
      ios{ios} {
    boost::system::error_code ec;
    if (is_verify) {
      context.set_default_verify_paths(ec);
      if (ec) throw AbradeException{"ssl default verify path", ec};
    }
    const auto verify_mode = is_verify
                               ? boost::asio::ssl::verify_peer | boost::asio::ssl::verify_fail_if_no_peer_cert
                               : boost::asio::ssl::verify_none;
    context.set_verify_mode(verify_mode);
  }

  auto connect(
    boost::asio::ip::tcp::socket& sock,
    const boost::asio::yield_context& yield) {
    auto result = std::make_unique<Connection<boost::asio::ssl::stream<boost::asio::ip::tcp::socket&>>>(
      [&yield, st=sensitive_teardown](auto& stream) {
      boost::system::error_code ec;
      stream.async_shutdown(yield[ec]);
      if (st && ec && ec != boost::asio::error::eof) throw AbradeException{"shutdown error", ec};
    },
      sock, context);

    boost::asio::ip::tcp::resolver resolver{ios};
    const auto lookup_result = await_resolver_with_timeout<boost::asio::ip::tcp::resolver::results_type>(
      resolver,
      "resolve host",
      yield,
      [this, &resolver](auto token) { return resolver.async_resolve(endpoint.host, endpoint.service, token); }
    );
    await_stream_with_timeout(sock, "ssl connect", yield, [&sock, &lookup_result](auto token) {
      boost::asio::async_connect(sock, lookup_result, token);
    });

    await_stream_with_timeout(result->get(), "ssl handshake", yield, [&result](auto token) {
      result->get().async_handshake(boost::asio::ssl::stream_base::client, token);
    });

    return std::move(result);
  }

private:
  const bool sensitive_teardown;
  HostEndpoint endpoint;
  boost::asio::ssl::context context;
  boost::asio::io_context& ios;
};

struct ProxiedConnection {
  ProxiedConnection(const std::string& proxy, const std::string& host_name, bool sensitive_teardown,
                    boost::asio::io_context& ios)
    : sensitive_teardown{sensitive_teardown},
      endpoint{parse_host_endpoint(host_name, "80", 80)},
      proxy_endpoint{parse_host_endpoint(proxy, "1080", 1080)},
      ios{ios} { }

  auto connect(
    boost::asio::ip::tcp::socket& sock,
    const boost::asio::yield_context& yield
  ) {
    auto result = std::make_unique<Connection<boost::asio::ip::tcp::socket&>>(
      [st=sensitive_teardown](auto& stream) {
      boost::system::error_code shutdown_error;
      stream.shutdown(boost::asio::ip::tcp::socket::shutdown_both, shutdown_error);
      if (st && shutdown_error && shutdown_error != boost::asio::error::eof)
        throw AbradeException{
          "tcp shutdown", shutdown_error
        };
    },
      sock);

    boost::asio::ip::tcp::resolver resolver{ios};
    const auto proxy_lookup = await_resolver_with_timeout<boost::asio::ip::tcp::resolver::results_type>(
      resolver,
      "resolve proxy",
      yield,
      [this, &resolver](auto token) {
        return resolver.async_resolve(proxy_endpoint.host, proxy_endpoint.service, token);
      }
    );
    await_stream_with_timeout(result->get(), "proxy connect", yield, [&result, &proxy_lookup](auto token) {
      boost::asio::async_connect(result->get(), proxy_lookup, token);
    });

    static const std::array<unsigned char, 3> auth_request{5, 1, 0};
    await_stream_with_timeout(result->get(), "proxy write auth", yield, [&result](auto token) {
      boost::asio::async_write(result->get(), boost::asio::buffer(auth_request.data(), auth_request.size()), token);
    });

    std::array<char, 2> auth_response;
    await_stream_with_timeout(result->get(), "proxy read auth", yield, [&result, &auth_response](auto token) {
      boost::asio::async_read(result->get(), boost::asio::buffer(auth_response.data(), auth_response.size()), token);
    });

    if (auth_response[0] != 5) {
      std::string err_msg{"SOCKS version "};
      err_msg.append(std::to_string(auth_response[0]));
      err_msg.append(" not supported.");
      throw AbradeException{std::move(err_msg)};
    }
    if (auth_response[1] != 0) {
      std::string err_msg{"SOCKS authentication "};
      err_msg.append(std::to_string(auth_response[1]));
      err_msg.append(" not supported.");
      throw AbradeException{std::move(err_msg)};
    }

    std::vector<unsigned char> connect_request{
      0x05, // Version
      0x01, // Connect
      0x00, // Reserved
      0x03, // Domain
      static_cast<unsigned char>(endpoint.host.size())
    };
    for (auto& name_byte : endpoint.host) { connect_request.emplace_back(name_byte); }
    connect_request.emplace_back(static_cast<unsigned char>((endpoint.port >> 8) & 0xFF));
    connect_request.emplace_back(static_cast<unsigned char>(endpoint.port & 0xFF));
    await_stream_with_timeout(result->get(), "proxy connection", yield, [&result, &connect_request](auto token) {
      boost::asio::async_write(result->get(), boost::asio::buffer(connect_request.data(), connect_request.size()), token);
    });

    std::array<char, 10> connect_response;
    await_stream_with_timeout(result->get(), "proxy read", yield, [&result, &connect_response](auto token) {
      boost::asio::async_read(result->get(), boost::asio::buffer(connect_response.data(), connect_response.size()), token);
    });

    if (connect_response[1] != 0) {
      std::string err_msg{"SOCKS connection failed:"};
      err_msg.append(std::to_string(connect_response[1]));
      throw AbradeException{std::move(err_msg)};
    }

    return std::move(result);
  }

private:
  const bool sensitive_teardown;
  HostEndpoint endpoint;
  HostEndpoint proxy_endpoint;
  boost::asio::io_context& ios;
};

struct ProxiedTlsConnection {
  ProxiedTlsConnection(const std::string& proxy, const std::string& host_name, bool is_verify,
                       bool sensitive_teardown,
                       boost::asio::io_context& ios)
    : sensitive_teardown{sensitive_teardown},
      endpoint{parse_host_endpoint(host_name, "443", 443)},
      proxy_endpoint{parse_host_endpoint(proxy, "1080", 1080)},
      context{boost::asio::ssl::context::tls_client},
      ios{ios} {
    boost::system::error_code ec;
    if (is_verify) {
      context.set_default_verify_paths(ec);
      if (ec) throw AbradeException{"default CA path set", ec};
    }
    const auto verify_mode = is_verify
                               ? boost::asio::ssl::verify_peer | boost::asio::ssl::verify_fail_if_no_peer_cert
                               : boost::asio::ssl::verify_none;
    context.set_verify_mode(verify_mode);
  }

  auto connect(
    boost::asio::ip::tcp::socket& sock,
    const boost::asio::yield_context& yield
  ) {
    auto result = std::make_unique<Connection<boost::asio::ssl::stream<boost::asio::ip::tcp::socket&>>>(
      [&yield, st=sensitive_teardown](auto& stream) {
      boost::system::error_code ec;
      stream.async_shutdown(yield[ec]);
      if (st && ec && ec != boost::asio::error::eof) throw AbradeException{"proxied ssl shutdown", ec};
    },
      sock, context);

    boost::asio::ip::tcp::resolver resolver{ios};
    const auto proxy_lookup = await_resolver_with_timeout<boost::asio::ip::tcp::resolver::results_type>(
      resolver,
      "resolve proxy",
      yield,
      [this, &resolver](auto token) {
        return resolver.async_resolve(proxy_endpoint.host, proxy_endpoint.service, token);
      }
    );
    await_stream_with_timeout(sock, "proxy connect", yield, [&sock, &proxy_lookup](auto token) {
      boost::asio::async_connect(sock, proxy_lookup, token);
    });

    static const std::array<unsigned char, 3> auth_request{5, 1, 0};
    await_stream_with_timeout(sock, "proxy write auth", yield, [&sock](auto token) {
      boost::asio::async_write(sock, boost::asio::buffer(auth_request.data(), auth_request.size()), token);
    });

    std::array<char, 2> auth_response;
    await_stream_with_timeout(sock, "proxy read auth", yield, [&sock, &auth_response](auto token) {
      boost::asio::async_read(sock, boost::asio::buffer(auth_response.data(), auth_response.size()), token);
    });

    if (auth_response[0] != 5) {
      std::string err_msg{"SOCKS version "};
      err_msg.append(std::to_string(auth_response[0]));
      err_msg.append(" not supported.");
      throw AbradeException{std::move(err_msg)};
    }
    if (auth_response[1] != 0) {
      std::string err_msg{"SOCKS authentication "};
      err_msg.append(std::to_string(auth_response[1]));
      err_msg.append(" not supported.");
      throw AbradeException{std::move(err_msg)};
    }

    std::vector<unsigned char> connect_request{
      0x05, // Version
      0x01, // Connect
      0x00, // Reserved
      0x03, // Domain
      static_cast<unsigned char>(endpoint.host.size())
    };
    for (auto& name_byte : endpoint.host) { connect_request.emplace_back(name_byte); }
    connect_request.emplace_back(static_cast<unsigned char>((endpoint.port >> 8) & 0xFF));
    connect_request.emplace_back(static_cast<unsigned char>(endpoint.port & 0xFF));
    await_stream_with_timeout(sock, "proxy connection", yield, [&sock, &connect_request](auto token) {
      boost::asio::async_write(sock, boost::asio::buffer(connect_request.data(), connect_request.size()), token);
    });

    std::array<char, 10> connect_response;
    await_stream_with_timeout(sock, "proxy read", yield, [&sock, &connect_response](auto token) {
      boost::asio::async_read(sock, boost::asio::buffer(connect_response.data(), connect_response.size()), token);
    });

    if (connect_response[1] != 0) {
      std::string err_msg{"SOCKS connection failed:"};
      err_msg.append(std::to_string(connect_response[1]));
      throw AbradeException{std::move(err_msg)};
    }

    await_stream_with_timeout(result->get(), "proxied ssl handshake", yield, [&result](auto token) {
      result->get().async_handshake(boost::asio::ssl::stream_base::client, token);
    });

    return std::move(result);
  }

private:
  const bool sensitive_teardown;
  HostEndpoint endpoint;
  HostEndpoint proxy_endpoint;
  boost::asio::ssl::context context;
  boost::asio::io_context& ios;
};
