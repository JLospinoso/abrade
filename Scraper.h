#pragma once
#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/beast/core.hpp>
#include <string>
#include <boost/beast/http/parser.hpp>
#include <iostream>
#include <string>
#include <ostream>
#include <fstream>
#include "UriGenerator.h"
#include "Controller.h"

template <typename Query, typename Connection, typename RequestWriter>
struct Scraper {
  Scraper(Query&& query, Connection&& connection, RequestWriter&& writer, Controller& controller,
          boost::asio::io_service& ios, bool is_verbose, const std::string& error_path)
    : is_verbose{is_verbose},
      error_path{error_path},
      controller{controller},
      ios{ios},
      query {std::forward<Query>(query)},
      connection{std::forward<Connection>(connection)},
      writer{std::forward<RequestWriter>(writer)},
      active_coroutines{} { }

  void run(UriGenerator& generator) {
    spawn_coroutine(generator);
    ios.run();
  }

private:
  struct LifetimeCounter {
    LifetimeCounter(unsigned short& value) : value{value} { value++; }
    ~LifetimeCounter() { value--; }
  private:
    unsigned short& value;
  };

  void spawn_coroutine(UriGenerator& generator) {
    spawn(ios, [this, &generator](const boost::asio::yield_context& yield)
        {
          LifetimeCounter ctr{active_coroutines};
          while (const auto uri = generator.next()) {
            if (active_coroutines < controller.recommended_coroutines()) { spawn_coroutine(generator); }
            try { coroutine(yield, *uri); }
            catch (const std::exception& e) {
              std::ofstream file;
              file.exceptions(std::ios_base::failbit | std::ios_base::badbit);
              file.open(error_path, std::ofstream::out | std::ofstream::app);
              file << *uri << ": " << e.what() << std::endl;
              if (is_verbose) std::cerr << "[-] Exception: " << e.what() << std::endl;
            }
            controller.register_completion(active_coroutines);
            if (active_coroutines > controller.recommended_coroutines()) return;
          }
        });
  }

  void coroutine(const boost::asio::yield_context& yield, const std::string& target) {
    boost::asio::ip::tcp::socket sock{ios};
    auto instance = connection.connect(sock, target, yield);
    writer.make_request(instance->get(), query, target, yield);
    query.execute(instance->get(), target, yield);
  }

  bool is_verbose;
  std::string error_path;
  Controller& controller;
  boost::asio::io_service& ios;
  Query query;
  Connection connection;
  RequestWriter writer;
  unsigned short active_coroutines;
};
