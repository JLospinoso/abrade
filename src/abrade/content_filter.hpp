#pragma once

#include <algorithm>
#include <boost/regex.hpp>
#include <iterator>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace abrade {

namespace detail {
[[nodiscard]] inline bool regex_matches(std::string_view body, const boost::regex& pattern) {
#if defined(__clang_analyzer__)
  static_cast<void>(body);
  static_cast<void>(pattern);
  return false;
#else
  return boost::regex_search(body.begin(), body.end(), pattern);
#endif
}
} // namespace detail

/// Repeatable filters applied to successful GET response bodies before persistence.
///
/// Required filters are conjunctive: every literal and regular expression must match.
/// Rejected filters are also conjunctive as blockers: any matching literal or regular
/// expression prevents the body from being written.
struct ContentFilters {
  ContentFilters() = default;

  ContentFilters(std::vector<std::string> required_text, std::vector<std::string> rejected_text,
                 std::vector<std::string> required_regex_text,
                 std::vector<std::string> rejected_regex_text)
      : required_literals{std::move(required_text)}, rejected_literals{std::move(rejected_text)},
        required_patterns{std::move(required_regex_text)},
        rejected_patterns{std::move(rejected_regex_text)} {
    required_regexes.reserve(required_patterns.size());
    std::ranges::transform(required_patterns, std::back_inserter(required_regexes),
                           [](const auto& pattern) { return boost::regex{pattern}; });
    rejected_regexes.reserve(rejected_patterns.size());
    std::ranges::transform(rejected_patterns, std::back_inserter(rejected_regexes),
                           [](const auto& pattern) { return boost::regex{pattern}; });
  }

  /// Returns true when no body filters are configured.
  [[nodiscard]] bool empty() const noexcept {
    return required_literals.empty() && rejected_literals.empty() && required_regexes.empty() &&
           rejected_regexes.empty();
  }

  /// Returns true when a response body satisfies all configured filters.
  [[nodiscard]] bool accepts(std::string_view body) const {
    const auto body_contains = [body](const auto& text) { return body.contains(text); };
    const auto body_matches = [body](const auto& pattern) {
      return detail::regex_matches(body, pattern);
    };

    return std::ranges::all_of(required_literals, body_contains) &&
           std::ranges::all_of(required_regexes, body_matches) &&
           std::ranges::none_of(rejected_literals, body_contains) &&
           std::ranges::none_of(rejected_regexes, body_matches);
  }

private:
  std::vector<std::string> required_literals;
  std::vector<std::string> rejected_literals;
  std::vector<std::string> required_patterns;
  std::vector<std::string> rejected_patterns;
  std::vector<boost::regex> required_regexes;
  std::vector<boost::regex> rejected_regexes;
};
} // namespace abrade
