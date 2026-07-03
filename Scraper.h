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
#include "Controller.h"
#include "Candidate.h"
#include "ScraperRuntime.h"

template <typename Generator, typename Query, typename Connection, typename RequestWriter, typename ErrorLog>
struct Scraper {
  Scraper(Query&& query, Connection&& connection, RequestWriter&& writer, ErrorLog&& error_log, Controller& controller,
          boost::asio::io_context& ios)
    : error_log{std::forward<ErrorLog>(error_log)},
      controller{controller},
      ios{ios},
      query {std::forward<Query>(query)},
      connection{std::forward<Connection>(connection)},
      writer{std::forward<RequestWriter>(writer)},
      active_coroutines{} { }

  void run(Generator& generator) {
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

  void spawn_coroutine(Generator& generator) {
    spawn(ios, [this, &generator](const boost::asio::yield_context& yield)
        {
          LifetimeCounter ctr{active_coroutines};
          while (const auto uri = generator.next()) { 
            if (active_coroutines < controller.recommended_coroutines()) { spawn_coroutine(generator); }
            // TODO: Need to generate URI, headers, and contents. How to integrate patterns?
            auto candidate = make_candidate(*uri);
            try { coroutine(yield, candidate); }
            catch (const std::exception& e) {
              error_log.record(*uri, e);
            }
            controller.register_completion(active_coroutines);
            if (active_coroutines > controller.recommended_coroutines()) return;
          }
        }, boost::asio::detached);
  }

  void coroutine(const boost::asio::yield_context& yield, const Candidate& candidate) {
    boost::asio::ip::tcp::socket sock{ios};
    auto instance = connection.connect(sock, yield);
    //TODO: Program against candidate instead of just URI
    writer.make_request(instance->get(), query, candidate, yield);
    query.execute(instance->get(), candidate.description(), yield);
  }

  ErrorLog error_log;
  Controller& controller;
  boost::asio::io_context& ios;
  Query query;
  Connection connection;
  RequestWriter writer;
  unsigned short active_coroutines;
};
