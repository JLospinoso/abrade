#pragma once
#include <cstddef>
#include <string>
#include <stdexcept>

/// Parsed command-line options for one Abrade invocation.
struct Options {
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
  /// Returns the human-readable startup summary.
  std::string get_pretty_print() const noexcept;
  const std::string& get_help() const noexcept;
  const std::string& get_proxy() const noexcept;
  const std::string& get_host() const noexcept;
  const std::string& get_pattern() const noexcept;
  const std::string& get_output_path() const noexcept;
  const std::string& get_error_path() const noexcept;
  const std::string& get_user_agent() const noexcept;
  const std::string& get_screen() const noexcept;
  size_t get_initial_coroutines() const noexcept;
  size_t get_minimum_coroutines() const noexcept;
  size_t get_maximum_coroutines() const noexcept;
  size_t get_sample_size() const noexcept;
  size_t get_sample_interval() const noexcept;
private:
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
  std::string host, pattern, output_path, error_path, help_str, proxy, user_agent, screen;
};

/// Command-line parsing failure that includes the generated help text.
struct OptionsException : std::runtime_error {
  explicit OptionsException(const std::string& msg, const Options& options) : runtime_error{ msg + "\n" + options.get_help() } { }
};
