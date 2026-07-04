#include <abrade/options.hpp>
#include <boost/program_options.hpp>
#include <boost/regex.hpp>
#include <exception>
#include <sstream>
#include <string>

namespace abrade {

using namespace std;
using namespace boost::program_options;

namespace {
void validate_regex_options(const vector<string>& patterns, const char* option_name,
                            const Options& options) {
  for (const auto& pattern : patterns) {
    try {
      boost::regex{pattern};
    } catch (const boost::regex_error& e) {
      throw OptionsException{string{option_name} + " is not a valid regex: " + e.what(), options};
    }
  }
}
} // namespace

void Options::add_parser_options(options_description& description) {
  description.add_options()("host", value<string>(&host),
                            "host name, optionally host:port (eg example.com)")(
      "pattern", value<string>(&pattern)->default_value("/"),
      "format of URL (eg ?mynum={1:5}&myhex=0x{hhhh}). See documentation for formatting of "
      "patterns.")(
      "agent",
      value<string>(&user_agent)
          ->default_value(
              "Mozilla/5.0 (Windows NT 6.1; Win64; x64; rv:47.0) Gecko/20100101 Firefox/47.0"),
      "User-agent string (default: Firefox 47)")(
      "out", value<string>(&output_path)->default_value(""),
      "output path. dir if contents enabled. (default: HOSTNAME)")(
      "err", value<string>(&error_path)->default_value(""),
      "error path (file). (default: HOSTNAME-err.log)")(
      "proxy", value<string>(&proxy)->default_value(""),
      "SOCKS5 proxy address:port. (default: none)")(
      "screen", value<string>(&screen)->default_value(""),
      "alias for --reject. omits 2xx response bodies containing text (default: none)")(
      "require", value<vector<string>>(&required_literals)->composing(),
      "require text in 2xx GET body before writing it. repeatable. requires --contents")(
      "reject", value<vector<string>>(&rejected_literals)->composing(),
      "reject 2xx GET body containing text before writing it. repeatable. requires --contents")(
      "require-regex", value<vector<string>>(&required_regexes)->composing(),
      "require regex match in 2xx GET body before writing it. repeatable. requires --contents")(
      "reject-regex", value<vector<string>>(&rejected_regexes)->composing(),
      "reject 2xx GET body matching regex before writing it. repeatable. requires --contents")(
      "follow-redirects", bool_switch(&follow_redirects),
      "follow same-scheme, same-authority redirects (default: no)")(
      "max-redirects", value<size_t>(&max_redirects),
      "maximum redirect hops when --follow-redirects is enabled (default: 5)")(
      "stdin,d", bool_switch(&from_stdin),
      "read from stdin (default: no)")("tls,t", bool_switch(&tls), "use tls/ssl (default: no)")(
      "sensitive,s", bool_switch(&sensitive_teardown),
      "complain about rude TCP teardowns (default: no)")(
      "tor,o", bool_switch(&tor), "use local proxy at 127.0.0.1:9050 (default: no)")(
      "verify,r", bool_switch(&verify), "verify tls/ssl peer (default: no)")(
      "leadzero,l", bool_switch(&leading_zeros), "output leading zeros in URL (default: no)")(
      "telescoping,e", bool_switch(&telescoping), "telescope implicit patterns (default: no)")(
      "found,f", bool_switch(&print_found),
      "print when resource found (default: no). implied by verbose")(
      "verbose,v", bool_switch(&verbose), "prints gratuitous output to console (default: no)")(
      "contents,c", bool_switch(&contents), "read full contents (default: no)")(
      "test", bool_switch(&test),
      "no network requests, just write generated URIs to console (default: no)")(
      "optimize,p", bool_switch(&optimize),
      "Optimize number of simultaneous requests (default: no)")(
      "init,i", value<size_t>(&initial_coroutines)->default_value(1000),
      "Initial number of simultaneous requests")(
      "min", value<size_t>(&minimum_coroutines)->default_value(1),
      "Minimum number of simultaneous requests")(
      "max", value<size_t>(&maximum_coroutines)->default_value(25000),
      "Maximum number of simultaneous requests")(
      "ssize", value<size_t>(&sample_size)->default_value(50), "Size of velocity sliding window")(
      "sint", value<size_t>(&sample_interval)->default_value(1000),
      "Size of sampling interval")("help,h", "produce help message");
}

void Options::validate_parsed_options(bool max_redirects_provided) {
  if (host.empty()) {
    throw OptionsException{"you must supply a host.", *this};
  }
  if (!from_stdin && pattern.empty()) {
    throw OptionsException{"you must supply a pattern.", *this};
  }
  if (initial_coroutines < 1) {
    throw OptionsException{"init must be positive", *this};
  }
  if (minimum_coroutines < 1) {
    throw OptionsException{"min must be positive", *this};
  }
  if (maximum_coroutines < 1) {
    throw OptionsException{"max must be positive", *this};
  }
  if (sample_size < 1) {
    throw OptionsException{"ssize must be positive", *this};
  }
  if (sample_interval < 1) {
    throw OptionsException{"ssize must be positive", *this};
  }
  if (!screen.empty()) {
    rejected_literals.push_back(screen);
  }
  const auto has_filters = !required_literals.empty() || !rejected_literals.empty() ||
                           !required_regexes.empty() || !rejected_regexes.empty();
  if (has_filters && !contents) {
    throw OptionsException{"content filters require --contents", *this};
  }
  validate_regex_options(required_regexes, "--require-regex", *this);
  validate_regex_options(rejected_regexes, "--reject-regex", *this);
  if (max_redirects < 1) {
    throw OptionsException{"max-redirects must be positive", *this};
  }
  if (max_redirects_provided && !follow_redirects) {
    throw OptionsException{"max-redirects requires --follow-redirects", *this};
  }
}

Options::Options(int argc, const char** argv) {
  options_description description("Usage: abrade host pattern");
  add_parser_options(description);

  positional_options_description pos_description;
  pos_description.add("host", 1);
  pos_description.add("pattern", 1);

  stringstream ss;
  ss << description;
  help_str = ss.str();

  variables_map vm;
  try {
    store(command_line_parser(argc, argv).options(description).positional(pos_description).run(),
          vm);
    notify(vm);
  } catch (const error& e) {
    throw OptionsException{e.what(), *this};
  }

  help = vm.contains("help");
  if (help) {
    return;
  }
  validate_parsed_options(vm.contains("max-redirects"));
  tls |= verify;
  print_found |= verbose;
  if (output_path.empty()) {
    output_path = host;
    if (contents) {
      output_path += "-contents";
    }
  }
  if (error_path.empty()) {
    error_path = host + "-err.log";
  }
  if (!is_proxy() && tor) {
    proxy = "127.0.0.1:9050";
  }
}

string Options::get_pretty_print() const noexcept {
  stringstream ss;
  if (from_stdin) {
    ss << "[ ] Reading input from stdin\n";
  } else {
    ss << "[ ] Host: " << get_host() << "\n" << "[ ] Pattern: " << get_pattern() << "\n";
  }
  ss << "[ ] Include leading zeros: " << (is_leading_zeros() ? "Yes" : "No") << "\n"
     << "[ ] Telescope pattern: " << (is_telescoping() ? "Yes" : "No") << "\n"
     << "[ ] TLS/SSL: " << (is_tls() ? "Yes" : "No") << "\n"
     << "[ ] TLS/SSL Peer Verify: " << (is_verify() ? "Yes" : "No") << "\n"
     << "[ ] User Agent: " << get_user_agent() << "\n"
     << "[ ] Proxy: " << (is_proxy() ? get_proxy() : "No") << "\n"
     << "[ ] Contents: " << (is_contents() ? "Yes" : "No") << "\n"
     << "[ ] Body filters: "
     << (required_literals.empty() && rejected_literals.empty() && required_regexes.empty() &&
                 rejected_regexes.empty()
             ? "None"
             : "Configured")
     << "\n"
     << "[ ] Follow redirects: " << (is_follow_redirects() ? "Yes" : "No") << "\n"
     << "[ ] Max redirects: " << get_max_redirects() << "\n"
     << "[ ] Output: " << get_output_path() << "\n"
     << "[ ] Error Output: " << get_error_path() << "\n"
     << "[ ] Verbose: " << (is_verbose() ? "Yes" : "No") << "\n"
     << "[ ] Print found: " << (is_print_found() ? "Yes" : "No") << "\n"
     << "[ ] Initial connections: " << get_initial_coroutines() << "\n"
     << "[ ] Optimize connections: " << (is_optimizer() ? "Yes" : "No");
  if (!is_optimizer()) {
    ss << "\n"
       << "[ ] Connection range: [" << get_minimum_coroutines() << "," << get_maximum_coroutines()
       << "] " << "\n"
       << "[ ] Optimization sample size: " << get_sample_size() << "\n"
       << "[ ] Optimization sample interval: " << get_sample_interval();
  }
  return ss.str();
}

bool Options::is_stdin() const noexcept { return from_stdin; }

bool Options::is_leading_zeros() const noexcept { return leading_zeros; }

bool Options::is_test() const noexcept { return test; }

bool Options::is_follow_redirects() const noexcept { return follow_redirects; }

bool Options::is_help() const noexcept { return help; }

bool Options::is_verbose() const noexcept { return verbose; }

bool Options::is_contents() const noexcept { return contents; }

bool Options::is_print_found() const noexcept { return print_found; }

bool Options::is_proxy() const noexcept { return !proxy.empty(); }

bool Options::is_telescoping() const noexcept { return telescoping; }

bool Options::is_tls() const noexcept { return tls; }

bool Options::is_verify() const noexcept { return verify; }

bool Options::is_sensitive_teardown() const noexcept { return sensitive_teardown; }

bool Options::is_optimizer() const noexcept { return optimize; }

const string& Options::get_help() const noexcept { return help_str; }

const string& Options::get_pattern() const noexcept { return pattern; }

const string& Options::get_proxy() const noexcept { return proxy; }

const string& Options::get_output_path() const noexcept { return output_path; }

const string& Options::get_error_path() const noexcept { return error_path; }

const string& Options::get_host() const noexcept { return host; }

const string& Options::get_user_agent() const noexcept { return user_agent; }

const string& Options::get_screen() const noexcept { return screen; }

const vector<string>& Options::get_required_literals() const noexcept { return required_literals; }

const vector<string>& Options::get_rejected_literals() const noexcept { return rejected_literals; }

const vector<string>& Options::get_required_regexes() const noexcept { return required_regexes; }

const vector<string>& Options::get_rejected_regexes() const noexcept { return rejected_regexes; }

size_t Options::get_max_redirects() const noexcept { return max_redirects; }

size_t Options::get_initial_coroutines() const noexcept { return initial_coroutines; }

size_t Options::get_minimum_coroutines() const noexcept { return minimum_coroutines; }

size_t Options::get_maximum_coroutines() const noexcept { return maximum_coroutines; }

size_t Options::get_sample_size() const noexcept { return sample_size; }

size_t Options::get_sample_interval() const noexcept { return sample_interval; }
} // namespace abrade
