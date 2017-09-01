#include "Options.h"
#include <exception>
#include <string>
#include <boost/program_options.hpp>
#include <sstream>

using namespace std;
using namespace boost::program_options;

Options::Options(int argc, const char** argv)
  : help{}, tls{}, verify{}, contents{}, verbose{}, optimize{}, 
    print_found{}, tor{}, sensitive_teardown{}, leading_zeros{}, 
    test{}, telescoping{} {
  options_description description("Usage: abrade host pattern");
  description.add_options()
    ("host", value<string>(&host), "host name (eg example.com)")
    ("pattern", value<string>(&pattern)->default_value("/"), "format of URL (eg ?mynum={1:5}&myhex=0x{hhhh}). See documentation for formatting of patterns.")
    ("agent", value<string>(&user_agent)->default_value("Mozilla/5.0 (Windows NT 6.1; Win64; x64; rv:47.0) Gecko/20100101 Firefox/47.0"), "User-agent string (default: Firefox 47)")
    ("out", value<string>(&output_path)->default_value(""), "output path. dir if contents enabled. (default: HOSTNAME)")
    ("err", value<string>(&error_path)->default_value(""), "error path (file). (default: HOSTNAME-err.log)")
    ("proxy", value<string>(&proxy)->default_value(""), "SOCKS5 proxy address:port. (default: none)")
    ("tls,t", bool_switch(&tls), "use tls/ssl (default: no)")
    ("sensitive,s", bool_switch(&sensitive_teardown), "complain about rude TCP teardowns (default: no)")
    ("tor,o", bool_switch(&tor), "use local proxy at 127.0.0.1:9050 (default: no)")
    ("verify,r", bool_switch(&verify), "verify ssl (default: no)")
    ("leadzero,l", bool_switch(&leading_zeros), "output leading zeros in URL (default: no)")
    ("telescoping,e", bool_switch(&telescoping), "do not telescope the pattern (default: no)")
    ("found,f", bool_switch(&print_found), "print when resource found (default: no). implied by verbose")
    ("verbose,v", bool_switch(&verbose), "prints gratuitous output to console (default: no)")
    ("contents,c", bool_switch(&contents), "read full contents (default: no)")
    ("test", bool_switch(&test), "no network requests, just write generated URIs to console (default: no)")
    ("optimize,p", bool_switch(&optimize), "Optimize number of simultaneous requests (default: no)")
    ("init,i", value<size_t>(&initial_coroutines)->default_value(1000), "Initial number of simultaneous requests")
    ("min", value<size_t>(&minimum_coroutines)->default_value(1), "Minimum number of simultaneous requests")
    ("max", value<size_t>(&maximum_coroutines)->default_value(25000), "Maximum number of simultaneous requests")
    ("ssize", value<size_t>(&sample_size)->default_value(50), "Size of velocity sliding window")
    ("sint", value<size_t>(&sample_interval)->default_value(1000), "Size of sampling interval")
    ("help,h", "produce help message");
  positional_options_description pos_description;
  pos_description.add("host", 1);
  pos_description.add("pattern", 1);

  variables_map vm;
  store(command_line_parser(argc, argv)
        .options(description)
        .positional(pos_description)
        .run(), vm);
  notify(vm);

  help = vm.count("help") >= 1;
  stringstream ss;
  ss << description;
  help_str = ss.str();
  if (help) return;
  if (host.size() == 0) throw OptionsException{"you must supply a host.", *this};
  if (pattern.size() == 0) throw OptionsException{"you must supply a pattern.", *this };
  if (initial_coroutines < 1) throw OptionsException{"init must be positive", *this };
  if (minimum_coroutines < 1) throw OptionsException{"min must be positive", *this };
  if (maximum_coroutines < 1) throw OptionsException{"max must be positive", *this };
  if (sample_size < 1) throw OptionsException{"ssize must be positive", *this };
  if (sample_interval < 1) throw OptionsException{"ssize must be positive", *this };
  tls |= verify;
  print_found |= verbose;
  if (output_path.empty()) {
    output_path = host;
    if (contents) { output_path += "-contents"; }
  }
  if (error_path.empty()) { error_path = host + "-err.log"; }
  if (!is_proxy() && tor) { proxy = "127.0.0.1:9050"; }
}

string Options::get_pretty_print() const noexcept {
  stringstream ss;
  ss <<
    "[ ] Host: " << get_host() << "\n" <<
    "[ ] Pattern: " << get_pattern() << "\n" <<
    "[ ] Include leading zeros: " << (is_leading_zeros() ? "Yes" : "No") << "\n" <<
    "[ ] Telescope pattern: " << (is_telescoping() ? "Yes" : "No") << "\n" <<
    "[ ] TLS/SSL: " << (is_tls() ? "Yes" : "No") << "\n" <<
    "[ ] TLS/SSL Peer Verify: " << (is_verify() ? "Yes" : "No") << "\n" <<
    "[ ] User Agent: " << get_user_agent() << "\n" <<
    "[ ] Proxy: " << (is_proxy() ? get_proxy() : "No") << "\n" <<
    "[ ] Contents: " << (is_contents() ? "Yes" : "No") << "\n" <<
    "[ ] Output: " << get_output_path() << "\n" <<
    "[ ] Error Output: " << get_error_path() << "\n" <<
    "[ ] Verbose: " << (is_verbose() ? "Yes" : "No") << "\n" <<
    "[ ] Print found: " << (is_print_found() ? "Yes" : "No") << "\n" <<
    "[ ] Initial connections: " << get_initial_coroutines() << "\n" <<
    "[ ] Optimize connections: " << (is_optimizer() ? "Yes" : "No");
  if (!is_optimizer()) {
    ss << "\n" << 
      "[ ] Connection range: [" << get_minimum_coroutines() << "," << get_maximum_coroutines() << "] " << "\n" <<
      "[ ] Optimization sample size: " << get_sample_size() << "\n" <<
      "[ ] Optimization sample interval: " << get_sample_interval();
  }
  return ss.str();
}

bool Options::is_leading_zeros() const noexcept { return leading_zeros; }

bool Options::is_test() const noexcept { return test; }

bool Options::is_help() const noexcept { return help; }

bool Options::is_verbose() const noexcept { return verbose; }

bool Options::is_contents() const noexcept { return contents; }

bool Options::is_print_found() const noexcept { return print_found; }

bool Options::is_proxy() const noexcept { return proxy.size() > 0; }

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

size_t Options::get_initial_coroutines() const noexcept { return initial_coroutines; }

size_t Options::get_minimum_coroutines() const noexcept { return minimum_coroutines; }

size_t Options::get_maximum_coroutines() const noexcept { return maximum_coroutines; }

size_t Options::get_sample_size() const noexcept { return sample_size; }

size_t Options::get_sample_interval() const noexcept { return sample_interval; }
