#pragma once
#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

namespace boost::program_options {
class options_description;
} // namespace boost::program_options

namespace abrade {

/// Parsed and normalized command-line options for one Abrade invocation.
///
/// `Options` is the product boundary between CLI parsing and scraper wiring. It
/// stores raw values, derived values such as `--verify` implying TLS, generated
/// help text, and validation for numeric concurrency settings.
struct Options {
  /// Parses argv, applies derived defaults, and throws `OptionsException` on invalid input.
  Options(int argc, const char** argv);

  /// True when URI candidates are read from standard input instead of a pattern.
  bool is_stdin() const noexcept;
  /// True when help was requested and no scrape should be started.
  bool is_help() const noexcept;
  /// True when network requests should use TLS.
  bool is_tls() const noexcept;
  /// True when TLS peer verification is enabled.
  bool is_verify() const noexcept;
  /// True when Abrade should fetch and save full response bodies.
  bool is_contents() const noexcept;
  /// True when a SOCKS5 proxy should be used.
  bool is_proxy() const noexcept;
  /// True when detailed progress and response output should be printed.
  bool is_verbose() const noexcept;
  /// True when successful candidates should be printed.
  bool is_print_found() const noexcept;
  /// True when adaptive coroutine tuning is enabled.
  bool is_optimizer() const noexcept;
  /// True when TCP shutdown errors should be surfaced.
  bool is_sensitive_teardown() const noexcept;
  /// True when implicit range output should preserve leading zeros.
  bool is_leading_zeros() const noexcept;
  /// True when implicit range patterns should use telescoping expansion.
  bool is_telescoping() const noexcept;
  /// True when generated URIs should be printed without network requests.
  bool is_test() const noexcept;
  /// True when same-origin, same-scheme redirects should be followed.
  bool is_follow_redirects() const noexcept;

  /// Returns the human-readable startup summary.
  std::string get_pretty_print() const noexcept;
  /// Returns the parser-generated help text.
  const std::string& get_help() const noexcept;
  /// Returns the configured SOCKS5 proxy, or an empty string when no proxy is active.
  const std::string& get_proxy() const noexcept;
  /// Returns the target host authority, optionally including a port.
  const std::string& get_host() const noexcept;
  /// Returns the URI pattern; unused when `is_stdin()` is true.
  const std::string& get_pattern() const noexcept;
  /// Returns the output file for HEAD mode or output directory for GET contents mode.
  const std::string& get_output_path() const noexcept;
  /// Returns the append-only error log path.
  const std::string& get_error_path() const noexcept;
  /// Returns the HTTP User-Agent header value.
  const std::string& get_user_agent() const noexcept;
  /// Returns the optional legacy `--screen` body substring, also stored as a rejected literal.
  const std::string& get_screen() const noexcept;
  /// Returns literal body strings that every persisted GET response must contain.
  const std::vector<std::string>& get_required_literals() const noexcept;
  /// Returns literal body strings that prevent a GET response from being persisted.
  const std::vector<std::string>& get_rejected_literals() const noexcept;
  /// Returns regex body patterns that every persisted GET response must match.
  const std::vector<std::string>& get_required_regexes() const noexcept;
  /// Returns regex body patterns that prevent a GET response from being persisted.
  const std::vector<std::string>& get_rejected_regexes() const noexcept;
  /// Returns the maximum redirect hops followed for one generated candidate.
  size_t get_max_redirects() const noexcept;
  /// Returns the initial active coroutine recommendation.
  size_t get_initial_coroutines() const noexcept;
  /// Returns the lower bound for adaptive coroutine recommendations.
  size_t get_minimum_coroutines() const noexcept;
  /// Returns the upper bound for adaptive coroutine recommendations.
  size_t get_maximum_coroutines() const noexcept;
  /// Returns the number of velocity samples retained by the adaptive controller.
  size_t get_sample_size() const noexcept;
  /// Returns the completion interval between adaptive controller samples.
  size_t get_sample_interval() const noexcept;

private:
  void add_parser_options(boost::program_options::options_description& description);
  void validate_parsed_options(bool max_redirects_provided);

  size_t initial_coroutines{};
  size_t minimum_coroutines{};
  size_t maximum_coroutines{};
  size_t sample_size{};
  size_t sample_interval{};
  bool help{};
  bool tls{};
  bool verify{};
  bool contents{};
  bool verbose{};
  bool optimize{};
  bool print_found{};
  bool tor{};
  bool sensitive_teardown{};
  bool leading_zeros{};
  bool test{};
  bool telescoping{};
  bool from_stdin{};
  bool follow_redirects{};
  size_t max_redirects{5};
  std::string host, pattern, output_path, error_path, help_str, proxy, user_agent, screen;
  std::vector<std::string> required_literals;
  std::vector<std::string> rejected_literals;
  std::vector<std::string> required_regexes;
  std::vector<std::string> rejected_regexes;
};

/// Command-line parsing failure that includes parser-generated help text.
struct OptionsException : std::runtime_error {
  explicit OptionsException(const std::string& msg, const Options& options)
      : runtime_error{msg + "\n" + options.get_help()} {}
};
} // namespace abrade
