#pragma once
#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/spawn.hpp>
#include <string>
#include <array>
#include "Exception.h"
#include <utility>
#include <memory>

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
  PlaintextConnection(const std::string& host_name, bool sensitive_teardown, boost::asio::io_service& ios)
    : sensitive_teardown{sensitive_teardown} {
    boost::system::error_code ec;
    boost::asio::ip::tcp::resolver resolver{ios};
    lookup_result = resolver.resolve({host_name, "http"}, ec);
    if (ec) throw AbradeException{"resolve host", ec};
  }

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

    boost::system::error_code ec;
    async_connect(result->get(), lookup_result, yield[ec]);
    if (ec) throw AbradeException{"tcp connect", ec};

    return std::move(result);
  }

private:
  bool sensitive_teardown;
  boost::asio::ip::tcp::resolver::iterator lookup_result;
};

struct TlsConnection {
  TlsConnection(const std::string& host_name, bool is_verify, bool sensitive_teardown, boost::asio::io_service& ios)
    : sensitive_teardown{sensitive_teardown}, context{boost::asio::ssl::context::sslv23} {
    boost::system::error_code ec;
    context.set_default_verify_paths(ec);
    if (ec) throw AbradeException{"ssl default verify path", ec};
    const auto verify_mode = is_verify
                               ? boost::asio::ssl::verify_peer | boost::asio::ssl::verify_fail_if_no_peer_cert
                               : boost::asio::ssl::verify_none;
    context.set_verify_mode(verify_mode);
    boost::asio::ip::tcp::resolver resolver{ios};
    lookup_result = resolver.resolve({host_name, "https"}, ec);
    if (sensitive_teardown && ec) throw AbradeException{"resolve host", ec};
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

    boost::system::error_code ec;
    async_connect(sock, lookup_result, yield[ec]);
    if (ec) throw AbradeException{"ssl connect", ec};

    result->get().async_handshake(boost::asio::ssl::stream_base::client, yield[ec]);
    if (ec) throw AbradeException{"ssl handshake", ec};

    return std::move(result);
  }

private:
  const bool sensitive_teardown;
  boost::asio::ip::tcp::resolver::iterator lookup_result;
  boost::asio::ssl::context context;
};

struct ProxiedConnection {
  ProxiedConnection(const std::string& proxy, const std::string& host_name, bool sensitive_teardown,
                    boost::asio::io_service& ios)
    : sensitive_teardown{sensitive_teardown}, host_name{host_name} {
    boost::system::error_code ec;
    const auto colon_pos = proxy.find(':');
    if (std::string::npos == colon_pos) throw AbradeException("Proxy does not contain colon (:).");
    boost::asio::ip::tcp::resolver resolver{ios};
    const auto proxy_host = proxy.substr(0, colon_pos);
    const auto proxy_port = proxy.substr(colon_pos + 1, proxy.size());
    proxy_lookup = resolver.resolve({proxy_host, proxy_port}, ec);
    if (ec) throw AbradeException{"resolve proxy", ec};
  }

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

    boost::system::error_code ec;
    async_connect(result->get(), proxy_lookup, yield[ec]);
    if (ec) throw AbradeException{"proxy connect ", ec};

    static const std::array<unsigned char, 3> auth_request{5, 1, 0};
    async_write(result->get(), boost::asio::buffer(auth_request.data(), auth_request.size()), yield[ec]);
    if (ec) throw AbradeException{"proxy write auth", ec};

    std::array<char, 2> auth_response;
    async_read(result->get(), boost::asio::buffer(auth_response.data(), auth_response.size()), yield[ec]);
    if (ec) throw AbradeException{"proxy read auth", ec};

    if (auth_response[0] != 5) {
      std::string err_msg{"SOCKS version "};
      err_msg.append(std::to_string(auth_response[0]));
      err_msg.append(" not supported.");
      throw AbradeException{move(err_msg)};
    }
    if (auth_response[1] != 0) {
      std::string err_msg{"SOCKS authentication "};
      err_msg.append(std::to_string(auth_response[1]));
      err_msg.append(" not supported.");
      throw AbradeException{move(err_msg)};
    }

    std::vector<unsigned char> connect_request{
      0x05, // Version
      0x01, // Connect
      0x00, // Reserved
      0x03, // Domain
      static_cast<unsigned char>(host_name.size())
    };
    for (auto& name_byte : host_name) { connect_request.emplace_back(name_byte); }
    connect_request.emplace_back(0); // high port byte
    connect_request.emplace_back(80); // low port byte
    async_write(result->get(), boost::asio::buffer(connect_request.data(), connect_request.size()), yield[ec]);
    if (ec) throw AbradeException{"proxy connection", ec};

    std::array<char, 10> connect_response;
    async_read(result->get(), boost::asio::buffer(connect_response.data(), connect_response.size()), yield[ec]);
    if (ec) throw AbradeException{"proxy read", ec};

    if (auth_response[1] != 0) {
      std::string err_msg{"SOCKS connection failed:"};
      err_msg.append(std::to_string(auth_response[1]));
      throw AbradeException{move(err_msg)};
    }

    return std::move(result);
  }

private:
  const bool sensitive_teardown;
  const std::string host_name;
  boost::asio::ip::tcp::resolver::iterator proxy_lookup;
};

struct ProxiedTlsConnection {
  ProxiedTlsConnection(const std::string& proxy, const std::string& host_name, bool is_verify,
                       bool sensitive_teardown,
                       boost::asio::io_service& ios)
    : sensitive_teardown{sensitive_teardown}, host_name{host_name}, context {
        boost::asio::ssl::context::sslv23_client
      } {
    boost::system::error_code ec;
    context.set_default_verify_paths(ec);
    if (ec) throw AbradeException{"default CA path set", ec};
    const auto verify_mode = is_verify
                               ? boost::asio::ssl::verify_peer | boost::asio::ssl::verify_fail_if_no_peer_cert
                               : boost::asio::ssl::verify_none;
    context.set_verify_mode(verify_mode);
    const auto colon_pos = proxy.find(':');
    if (std::string::npos == colon_pos) throw AbradeException("Proxy does not contain colon (:).");
    boost::asio::ip::tcp::resolver resolver{ios};
    const auto proxy_host = proxy.substr(0, colon_pos);
    const auto proxy_port = proxy.substr(colon_pos + 1, proxy.size());
    proxy_lookup = resolver.resolve({proxy_host, proxy_port}, ec);
    if (ec) throw AbradeException{"resolve proxy", ec};
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

    boost::system::error_code ec;
    async_connect(sock, proxy_lookup, yield[ec]);
    if (ec) throw AbradeException{"proxy connect ", ec};

    static const std::array<unsigned char, 3> auth_request{5, 1, 0};
    async_write(sock, boost::asio::buffer(auth_request.data(), auth_request.size()), yield[ec]);
    if (ec) throw AbradeException{"proxy write auth", ec};

    std::array<char, 2> auth_response;
    async_read(sock, boost::asio::buffer(auth_response.data(), auth_response.size()), yield[ec]);
    if (ec) throw AbradeException{"proxy read auth", ec};

    if (auth_response[0] != 5) {
      std::string err_msg{"SOCKS version "};
      err_msg.append(std::to_string(auth_response[0]));
      err_msg.append(" not supported.");
      throw AbradeException{move(err_msg)};
    }
    if (auth_response[1] != 0) {
      std::string err_msg{"SOCKS authentication "};
      err_msg.append(std::to_string(auth_response[1]));
      err_msg.append(" not supported.");
      throw AbradeException{move(err_msg)};
    }

    std::vector<unsigned char> connect_request{
      0x05, // Version
      0x01, // Connect
      0x00, // Reserved
      0x03, // Domain
      static_cast<unsigned char>(host_name.size())
    };
    for (auto& name_byte : host_name) { connect_request.emplace_back(name_byte); }
    connect_request.emplace_back(0x01); // high port byte
    connect_request.emplace_back(0xBB); // low port byte
    async_write(sock, boost::asio::buffer(connect_request.data(), connect_request.size()), yield[ec]);
    if (ec) throw AbradeException{"proxy connection", ec};

    std::array<char, 10> connect_response;
    async_read(sock, boost::asio::buffer(connect_response.data(), connect_response.size()), yield[ec]);
    if (ec) throw AbradeException{"proxy read", ec};

    if (auth_response[1] != 0) {
      std::string err_msg{"SOCKS connection failed:"};
      err_msg.append(std::to_string(auth_response[1]));
      throw AbradeException{move(err_msg)};
    }

    result->get().async_handshake(boost::asio::ssl::stream_base::client, yield[ec]);
    if (ec) throw AbradeException{"proxied ssl handshake", ec};

    return std::move(result);
  }

private:
  const bool sensitive_teardown;
  const std::string& host_name;
  boost::asio::ip::tcp::resolver::iterator proxy_lookup;
  boost::asio::ssl::context context;
};
