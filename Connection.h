#pragma once
#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/spawn.hpp>
#include <algorithm>
#include <string>
#include <array>
#include "Exception.h"
#include "Endpoint.h"
#include "NetworkTimeout.h"
#include <functional>
#include <iostream>
#include <utility>
#include <iterator>
#include <memory>
#include <vector>

/// Owns connection teardown behavior for a stream borrowed or built by a connection policy.
template <typename Stream>
struct Connection {
  template <typename... Args>
  Connection(std::function<void(Stream&)>&& cleanup_callback, Args&&... args) : stream {std::forward<Args>(args)...},
                                                                                cleanup{std::move(cleanup_callback)} { }

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

/// Opens direct plaintext TCP connections.
struct PlaintextConnection {
  PlaintextConnection(const std::string& host_name, bool strict_teardown, boost::asio::io_context& io_context)
    : sensitive_teardown{strict_teardown},
      endpoint{parse_host_endpoint(host_name, "80", 80)},
      ios{io_context} { }

  /// Resolves the target host, connects the socket, and returns a managed plaintext stream.
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

    return result;
  }

private:
  bool sensitive_teardown;
  HostEndpoint endpoint;
  boost::asio::io_context& ios;
};

/// Opens direct TLS connections.
struct TlsConnection {
  TlsConnection(const std::string& host_name, bool is_verify, bool strict_teardown, boost::asio::io_context& io_context)
    : sensitive_teardown{strict_teardown},
      endpoint{parse_host_endpoint(host_name, "443", 443)},
      context{boost::asio::ssl::context::tls_client},
      ios{io_context} {
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

  /// Resolves the target host, connects the socket, completes TLS handshake, and returns a managed stream.
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

    return result;
  }

private:
  const bool sensitive_teardown;
  HostEndpoint endpoint;
  boost::asio::ssl::context context;
  boost::asio::io_context& ios;
};

/// Opens plaintext TCP connections through a SOCKS5 proxy.
struct ProxiedConnection {
  ProxiedConnection(const std::string& proxy, const std::string& host_name, bool strict_teardown,
                    boost::asio::io_context& io_context)
    : sensitive_teardown{strict_teardown},
      endpoint{parse_host_endpoint(host_name, "80", 80)},
      proxy_endpoint{parse_host_endpoint(proxy, "1080", 1080)},
      ios{io_context} { }

  /// Connects to the proxy, negotiates SOCKS5, and returns a managed plaintext stream to the target.
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

    std::array<unsigned char, 2> auth_response;
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
    std::ranges::transform(endpoint.host, std::back_inserter(connect_request),
                           [](char name_byte) { return static_cast<unsigned char>(name_byte); });
    connect_request.emplace_back(static_cast<unsigned char>((endpoint.port >> 8) & 0xFF));
    connect_request.emplace_back(static_cast<unsigned char>(endpoint.port & 0xFF));
    await_stream_with_timeout(result->get(), "proxy connection", yield, [&result, &connect_request](auto token) {
      boost::asio::async_write(result->get(), boost::asio::buffer(connect_request.data(), connect_request.size()), token);
    });

    std::array<unsigned char, 10> connect_response;
    await_stream_with_timeout(result->get(), "proxy read", yield, [&result, &connect_response](auto token) {
      boost::asio::async_read(result->get(), boost::asio::buffer(connect_response.data(), connect_response.size()), token);
    });

    if (connect_response[1] != 0) {
      std::string err_msg{"SOCKS connection failed:"};
      err_msg.append(std::to_string(connect_response[1]));
      throw AbradeException{std::move(err_msg)};
    }

    return result;
  }

private:
  const bool sensitive_teardown;
  HostEndpoint endpoint;
  HostEndpoint proxy_endpoint;
  boost::asio::io_context& ios;
};

/// Opens TLS connections through a SOCKS5 proxy.
struct ProxiedTlsConnection {
  ProxiedTlsConnection(const std::string& proxy, const std::string& host_name, bool is_verify,
                       bool strict_teardown,
                       boost::asio::io_context& io_context)
    : sensitive_teardown{strict_teardown},
      endpoint{parse_host_endpoint(host_name, "443", 443)},
      proxy_endpoint{parse_host_endpoint(proxy, "1080", 1080)},
      context{boost::asio::ssl::context::tls_client},
      ios{io_context} {
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

  /// Negotiates SOCKS5 through the proxy, performs TLS handshake, and returns a managed stream.
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

    std::array<unsigned char, 2> auth_response;
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
    std::ranges::transform(endpoint.host, std::back_inserter(connect_request),
                           [](char name_byte) { return static_cast<unsigned char>(name_byte); });
    connect_request.emplace_back(static_cast<unsigned char>((endpoint.port >> 8) & 0xFF));
    connect_request.emplace_back(static_cast<unsigned char>(endpoint.port & 0xFF));
    await_stream_with_timeout(sock, "proxy connection", yield, [&sock, &connect_request](auto token) {
      boost::asio::async_write(sock, boost::asio::buffer(connect_request.data(), connect_request.size()), token);
    });

    std::array<unsigned char, 10> connect_response;
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

    return result;
  }

private:
  const bool sensitive_teardown;
  HostEndpoint endpoint;
  HostEndpoint proxy_endpoint;
  boost::asio::ssl::context context;
  boost::asio::io_context& ios;
};
