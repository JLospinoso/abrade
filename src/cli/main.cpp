#include <abrade/action.hpp>
#include <abrade/connection.hpp>
#include <abrade/controller.hpp>
#include <abrade/generator.hpp>
#include <abrade/options.hpp>
#include <abrade/query.hpp>
#include <abrade/redirect_policy.hpp>
#include <abrade/run_stats.hpp>
#include <abrade/scraper.hpp>
#include <abrade/scraper_runtime.hpp>
#include <abrade/writer.hpp>
#include <iostream>
#include <utility>

using namespace std;
using namespace abrade;

namespace {
template <typename Generator, typename Query, typename Connection, typename RequestWriter>
void run_scraper(Generator&& generator, Query&& query, Connection&& connection,
                 Controller& controller, RequestWriter& writer, boost::asio::io_context& ios,
                 const Options& options, RunStats& stats) {
  Scraper<Generator, Query, Connection, RequestWriter, FileErrorLog> scraper{
      std::forward<Query>(query),
      std::forward<Connection>(connection),
      std::forward<RequestWriter>(writer),
      FileErrorLog{options.get_error_path(), options.is_verbose()},
      controller,
      ios,
      stats};
  scraper.run(std::forward<Generator>(generator));
}

Generator& make_generator(const Options& options) {
  if (options.is_stdin()) {
    static StdinGenerator stdin_generator{};
    return stdin_generator;
  }
  static UriGenerator uri_generator{options.get_pattern(), options.is_leading_zeros(),
                                    options.is_telescoping()};
  return uri_generator;
}

RedirectPolicy make_redirect_policy(const Options& options) {
  return RedirectPolicy{.follow = options.is_follow_redirects(),
                        .max_redirects = options.get_max_redirects(),
                        .scheme = options.is_tls() ? "https" : "http",
                        .authority = options.get_host()};
}

ContentFilters make_content_filters(const Options& options) {
  return ContentFilters{options.get_required_literals(), options.get_rejected_literals(),
                        options.get_required_regexes(), options.get_rejected_regexes()};
}

GetQuery make_get(const Options& options, RunStats& stats) {
  return GetQuery{GetAction{options.get_output_path(), make_content_filters(options),
                            options.is_verbose(), stats},
                  options.is_print_found(), options.is_verbose(), make_redirect_policy(options),
                  stats};
}

HeadQuery make_head(const Options& options, RunStats& stats) {
  return HeadQuery{HeadAction{options.get_output_path(), options.is_verbose()},
                   options.is_print_found(), options.is_verbose(), make_redirect_policy(options),
                   stats};
}
} // namespace

int main(int argc, const char** argv) {
  try {
    const Options options{argc, argv};
    if (options.is_help()) {
      cout << options.get_help() << '\n';
      return EXIT_SUCCESS;
    }
    cout << options.get_pretty_print() << '\n';
    RunStats stats;
    boost::asio::io_context ios;
    RequestWriter writer{options.get_host(), options.is_verbose(), options.get_user_agent()};
    FixedController fixed_controller{options.get_initial_coroutines(),
                                     options.get_sample_interval()};
    AdaptiveController adaptive_controller{
        options.get_initial_coroutines(), options.get_sample_size(), options.get_sample_interval(),
        options.get_minimum_coroutines(), options.get_maximum_coroutines()};
    auto& controller = options.is_optimizer() ? static_cast<Controller&>(adaptive_controller)
                                              : static_cast<Controller&>(fixed_controller);
    if (!options.is_stdin()) {
      const UriGenerator uri_generator{options.get_pattern(), options.is_leading_zeros(),
                                       options.is_telescoping()};
      try {
        const auto cardinality = uri_generator.get_range_size();
        cout << "[ ] URL generation set cardinality is " << cardinality << '\n';
      } catch (const overflow_error&) {
        cout << "[!] URL generation set log cardinality is " << uri_generator.get_log_range_size()
             << '\n';
      }
    }
    auto& generator = make_generator(options);
    if (options.is_test()) {
      cout << "[ ] TEST: Writing URIs to console" << '\n';
      const char* const prefix = options.is_tls() ? "https://" : "http://";
      while (const auto& uri = generator.next()) {
        cout << prefix << options.get_host() << *uri << '\n';
      }
      return EXIT_SUCCESS;
    }
    if (options.is_tls()) {
      if (options.is_proxy()) {
        auto connection =
            ProxiedTlsConnection{options.get_proxy(), options.get_host(), options.is_verify(),
                                 options.is_sensitive_teardown(), ios};
        if (options.is_contents()) {
          run_scraper(generator, make_get(options, stats), std::move(connection), controller,
                      writer, ios, options, stats);
        } else {
          run_scraper(generator, make_head(options, stats), std::move(connection), controller,
                      writer, ios, options, stats);
        }
      } else {
        auto connection = TlsConnection{options.get_host(), options.is_verify(),
                                        options.is_sensitive_teardown(), ios};
        if (options.is_contents()) {
          run_scraper(generator, make_get(options, stats), std::move(connection), controller,
                      writer, ios, options, stats);
        } else {
          run_scraper(generator, make_head(options, stats), std::move(connection), controller,
                      writer, ios, options, stats);
        }
      }
    } else {
      if (options.is_proxy()) {
        auto connection = ProxiedConnection{options.get_proxy(), options.get_host(),
                                            options.is_sensitive_teardown(), ios};
        if (options.is_contents()) {
          run_scraper(generator, make_get(options, stats), std::move(connection), controller,
                      writer, ios, options, stats);
        } else {
          run_scraper(generator, make_head(options, stats), std::move(connection), controller,
                      writer, ios, options, stats);
        }
      } else {
        auto connection =
            PlaintextConnection{options.get_host(), options.is_sensitive_teardown(), ios};
        if (options.is_contents()) {
          run_scraper(generator, make_get(options, stats), std::move(connection), controller,
                      writer, ios, options, stats);
        } else {
          run_scraper(generator, make_head(options, stats), std::move(connection), controller,
                      writer, ios, options, stats);
        }
      }
    }
    cout << stats.summary() << '\n';
    return stats.has_errors() ? EXIT_FAILURE : EXIT_SUCCESS;
  } catch (const OptionsException& e) {
    cerr << "[-] " << e.what() << '\n';
    return 2;
  } catch (const exception& e) {
    cerr << "[-] Unknown error occurred: " << e.what() << '\n';
    return EXIT_FAILURE;
  }
}
