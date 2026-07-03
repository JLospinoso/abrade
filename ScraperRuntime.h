#pragma once

#include <exception>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include "Candidate.h"

/// Builds the currently supported candidate shape from a generated URI path.
inline Candidate make_candidate(std::string uri) {
  return Candidate{
    std::move(uri),
    {},
    {}
  };
}

/// Append-only scraper error log used to retain candidate context for failures.
struct FileErrorLog {
  FileErrorLog(std::string log_path, bool verbose_output)
    : path{std::move(log_path)},
      verbose{verbose_output} { }

  /// Records the candidate URI and exception message; verbose mode also writes to stderr.
  void record(std::string_view uri, const std::exception& error) const {
    std::ofstream file;
    file.exceptions(std::ios_base::failbit | std::ios_base::badbit);
    file.open(path, std::ofstream::out | std::ofstream::app);
    file << uri << ": " << error.what() << std::endl;
    if (verbose) {
      std::cerr << "[-] Exception: " << error.what() << std::endl;
    }
  }

private:
  std::string path;
  bool verbose;
};
