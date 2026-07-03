#pragma once

#include <charconv>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>

/// Parsed host authority with the host, service string, and numeric TCP port.
struct HostEndpoint {
  std::string authority;
  std::string host;
  std::string service;
  std::uint16_t port{};
};

/// Parses a TCP port from decimal text and rejects empty, partial, or out-of-range values.
inline std::uint16_t parse_tcp_port(std::string_view text) {
  if (text.empty()) {
    throw std::runtime_error{"Host port is empty."};
  }
  unsigned int parsed{};
  const auto* begin = text.data();
  const auto* end = text.data() + text.size();
  const auto [ptr, ec] = std::from_chars(begin, end, parsed);
  if (ec != std::errc{} || ptr != end || parsed == 0 || parsed > 65535) {
    throw std::runtime_error{"Host port is invalid."};
  }
  return static_cast<std::uint16_t>(parsed);
}

/// Parses host authorities such as example.com, example.com:8443, ::1, or [::1]:8443.
inline HostEndpoint parse_host_endpoint(std::string_view authority,
                                        std::string_view default_service,
                                        std::uint16_t default_port) {
  if (authority.empty()) {
    throw std::runtime_error{"Host is empty."};
  }

  std::string_view host = authority;
  std::string_view service = default_service;
  auto port = default_port;

  if (authority.front() == '[') {
    const auto close = authority.find(']');
    if (close == std::string_view::npos) {
      throw std::runtime_error{"Bracketed IPv6 host is missing closing bracket."};
    }
    host = authority.substr(1, close - 1);
    const auto rest = authority.substr(close + 1);
    if (!rest.empty()) {
      if (rest.front() != ':') {
        throw std::runtime_error{"Bracketed IPv6 host has invalid suffix."};
      }
      service = rest.substr(1);
      port = parse_tcp_port(service);
    }
  } else {
    const auto first_colon = authority.find(':');
    const auto last_colon = authority.rfind(':');
    if (first_colon != std::string_view::npos && first_colon == last_colon) {
      host = authority.substr(0, first_colon);
      service = authority.substr(first_colon + 1);
      port = parse_tcp_port(service);
    }
  }

  if (host.empty()) {
    throw std::runtime_error{"Host is empty."};
  }

  return HostEndpoint{
    std::string{authority},
    std::string{host},
    std::string{service},
    port
  };
}
