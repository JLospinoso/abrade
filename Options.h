#pragma once
#include <string>
#include <stdexcept>

struct Options {
  Options(int argc, const char** argv);
  bool is_help() const noexcept;
  bool is_tls() const noexcept;
  bool is_verify() const noexcept;
  bool is_contents() const noexcept;
  bool is_proxy() const noexcept;
  bool is_verbose() const noexcept;
  bool is_print_found() const noexcept;
  bool is_optimizer() const noexcept;
  bool is_sensitive_teardown() const noexcept;
  bool is_leading_zeros() const noexcept;
  bool is_telescoping() const noexcept;
  bool is_test() const noexcept;
  std::string get_pretty_print() const noexcept;
  const std::string& get_help() const noexcept;
  const std::string& get_proxy() const noexcept;
  const std::string& get_host() const noexcept;
  const std::string& get_pattern() const noexcept;
  const std::string& get_output_path() const noexcept;
  const std::string& get_error_path() const noexcept;
  const std::string& get_user_agent() const noexcept;
  size_t get_initial_coroutines() const noexcept;
  size_t get_minimum_coroutines() const noexcept;
  size_t get_maximum_coroutines() const noexcept;
  size_t get_sample_size() const noexcept;
  size_t get_sample_interval() const noexcept;
private:
  size_t initial_coroutines, minimum_coroutines, maximum_coroutines, sample_size, sample_interval;
  bool help, tls, verify, contents, verbose, optimize, print_found, tor, sensitive_teardown, 
        leading_zeros, test, telescoping;
  std::string host, pattern, output_path, error_path, help_str, proxy, user_agent;
};

struct OptionsException : std::runtime_error {
  OptionsException(const std::string& msg, const Options& options) : runtime_error{ msg + "\n" + options.get_help() } { }
};
