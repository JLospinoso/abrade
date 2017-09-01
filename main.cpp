#include "Options.h"
#include "UriGenerator.h"
#include "Scraper.h"
#include <iostream>
#include "Query.h"
#include <utility>
#include "Connection.h"
#include "Action.h"
#include "Writer.h"
#include "Controller.h"

using namespace std;

namespace {
  template <typename Query, typename Connection, typename RequestWriter>
  void run_scraper(UriGenerator& generator, Query&& query, Connection&& connection,
                   Controller& controller, RequestWriter& writer, boost::asio::io_service& ios,
                   const Options& options) {
    Scraper<Query, Connection, RequestWriter> scraper{
      std::forward<Query>(query), std::forward<Connection>(connection), std::forward<RequestWriter>(writer), controller,
      ios,
      options.is_verbose(), options.get_error_path()
    };
    scraper.run(generator);
  }

  GetQuery make_get(const Options& options) {
    return GetQuery{
      GetAction{ options.get_output_path(), options.is_verbose() }, options.is_print_found(), options.is_verbose()
    };
  }

  HeadQuery make_head(const Options& options) {
    return HeadQuery{
      HeadAction{ options.get_output_path(), options.is_verbose() }, options.is_print_found(), options.is_verbose()
    };
  }
}

int main(int argc, const char** argv) {
  try {
    Options options{argc, argv};
    if (options.is_help()) {
      cout << options.get_help() << endl;
      return EXIT_SUCCESS;
    }
    cout << options.get_pretty_print() << endl;
    boost::asio::io_service ios;
    RequestWriter writer{options.get_host(), options.is_verbose(), options.get_user_agent()};
    FixedController fixed_controller{options.get_initial_coroutines(), options.get_sample_interval()};
    AdaptiveController adaptive_controller{
      options.get_initial_coroutines(), options.get_sample_size(),
      options.get_sample_interval(), options.get_minimum_coroutines(), options.get_maximum_coroutines()
    };
    auto& controller = options.is_optimizer()
                         ? static_cast<Controller&>(adaptive_controller)
                         : static_cast<Controller&>(fixed_controller);
    UriGenerator generator{ options.get_pattern(), options.is_leading_zeros(), options.is_telescoping() };
    try {
      auto cardinality = generator.get_range_size();
      cout << "[ ] URL generation set cardinality is " << cardinality << endl;
    } catch (const overflow_error&) {
      cout << "[!] URL generation set log cardinality is " << generator.get_log_range_size() << endl;      
    }
    if (options.is_test()) {
      cout << "[ ] TEST: Writing URIs to console" << endl;
      const auto prefix = options.is_tls() ? "https://" : "http://";
      while(const auto& uri = generator.next()) {
        cout << prefix << options.get_host() << *uri << endl;
      }
      return EXIT_SUCCESS;
    }
    if (options.is_tls()) {
      if (options.is_proxy()) {
        auto connection = ProxiedTlsConnection{options.get_proxy(), options.get_host(), options.is_verify(), options.is_sensitive_teardown(), ios};
        if (options.is_contents()) {
          run_scraper(generator,
                      make_get(options),
                      move(connection),
                      controller,
                      writer,
                      ios,
                      options);
        }
        else {
          run_scraper(generator,
            make_head(options),
                      move(connection),
                      controller,
                      writer,
                      ios,
                      options);
        }
      }
      else {
        auto connection = TlsConnection{options.get_host(), options.is_verify(), options.is_sensitive_teardown(), ios};
        if (options.is_contents()) {
          run_scraper(generator,
            make_get(options),
                      move(connection),
                      controller,
                      writer,
                      ios,
                      options);
        }
        else {
          run_scraper(generator,
            make_head(options),
                      move(connection),
                      controller,
                      writer,
                      ios,
                      options);
        }
      }
    }
    else {
      if (options.is_proxy()) {
        auto connection = ProxiedConnection{options.get_proxy(), options.get_host(), options.is_sensitive_teardown(), ios};
        if (options.is_contents()) {
          run_scraper(generator,
            make_get(options),
                      move(connection),
                      controller,
                      writer,
                      ios,
                      options);
        }
        else {
          run_scraper(generator,
            make_head(options),
                      move(connection),
                      controller,
                      writer,
                      ios,
                      options);
        }
      }
      else {
        auto connection = PlaintextConnection{options.get_host(), options.is_sensitive_teardown(), ios};
        if (options.is_contents()) {
          run_scraper(generator,
            make_get(options),
                      move(connection),
                      controller,
                      writer,
                      ios,
                      options);
        }
        else {
          run_scraper(generator,
                      make_head(options),
                      move(connection),
                      controller,
                      writer,
                      ios,
                      options);
        }
      }
    }
  }
  catch (OptionsException& e) { cerr << "[-] " << e.what() << endl; }
  catch (exception& e) {
    cerr << "[-] Unknown error occurred: " << e.what() << endl;
  }
  return EXIT_SUCCESS;
}
