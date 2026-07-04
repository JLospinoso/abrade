#pragma once

#include <boost/beast/http.hpp>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace abrade {

/// Opt-in redirect policy for conservative same-origin HTTP redirects.
struct RedirectPolicy {
  bool follow{};
  std::size_t max_redirects{5};
  std::string scheme;
  std::string authority;

  /// Returns a request target when a response should be followed.
  template <typename Fields>
  [[nodiscard]] std::optional<std::string> redirect_target(unsigned int status_code,
                                                           const Fields& fields,
                                                           std::string_view current_target) const {
    if (!follow || !is_redirect_status(status_code)) {
      return std::nullopt;
    }

    const auto location = fields[boost::beast::http::field::location];
    if (location.empty()) {
      return std::nullopt;
    }
    return normalize_location(std::string_view{location.data(), location.size()}, current_target);
  }

private:
  [[nodiscard]] static bool is_redirect_status(unsigned int status_code) noexcept {
    return status_code == 301U || status_code == 302U || status_code == 303U ||
           status_code == 307U || status_code == 308U;
  }

  [[nodiscard]] static std::string_view without_fragment(std::string_view location) noexcept {
    const auto fragment = location.find('#');
    if (fragment == std::string_view::npos) {
      return location;
    }
    return location.substr(0, fragment);
  }

  [[nodiscard]] static std::string resolve_relative(std::string_view location,
                                                    std::string_view current_target) {
    if (location.starts_with('/')) {
      return std::string{location};
    }
    if (location.starts_with('?')) {
      const auto query = current_target.find('?');
      const auto base =
          query == std::string_view::npos ? current_target : current_target.substr(0, query);
      return std::string{base} + std::string{location};
    }

    const auto query = current_target.find('?');
    const auto path =
        query == std::string_view::npos ? current_target : current_target.substr(0, query);
    const auto last_slash = path.rfind('/');
    const auto directory = last_slash == std::string_view::npos ? std::string_view{"/"}
                                                                : path.substr(0, last_slash + 1);
    return std::string{directory} + std::string{location};
  }

  [[nodiscard]] std::optional<std::string>
  normalize_location(std::string_view raw_location, std::string_view current_target) const {
    const auto location = without_fragment(raw_location);
    if (location.empty() || location.starts_with('#')) {
      return std::nullopt;
    }

    const auto scheme_separator = location.find("://");
    if (scheme_separator != std::string_view::npos) {
      const auto location_scheme = location.substr(0, scheme_separator);
      if (location_scheme != scheme) {
        return std::nullopt;
      }
      return normalize_absolute(location.substr(scheme_separator + 3));
    }

    if (location.starts_with("//")) {
      return normalize_absolute(location.substr(2));
    }

    return resolve_relative(location, current_target);
  }

  [[nodiscard]] std::optional<std::string>
  normalize_absolute(std::string_view authority_and_path) const {
    const auto path_begin = authority_and_path.find_first_of("/?");
    const auto location_authority = path_begin == std::string_view::npos
                                        ? authority_and_path
                                        : authority_and_path.substr(0, path_begin);
    if (location_authority != authority) {
      return std::nullopt;
    }
    if (path_begin == std::string_view::npos) {
      return std::string{"/"};
    }
    if (authority_and_path[path_begin] == '?') {
      return std::string{"/"} + std::string{authority_and_path.substr(path_begin)};
    }
    return std::string{authority_and_path.substr(path_begin)};
  }
};
} // namespace abrade
