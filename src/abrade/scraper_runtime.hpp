#pragma once

#include <abrade/candidate.hpp>
#include <exception>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>

namespace abrade {

/// Builds the currently supported candidate shape from a generated URI path.
///
/// This adapter is the narrow point between generators and request candidates.
/// It should absorb future candidate enrichment before `Scraper` does.
inline Candidate make_candidate(std::string uri) { return Candidate{std::move(uri), {}, {}}; }

/// Append-only scraper error log used to retain candidate context for failures.
///
/// The scraper catches per-candidate exceptions, records the URI plus exception
/// text, and keeps processing other candidates.
struct FileErrorLog {
  FileErrorLog(std::string log_path, bool verbose_output)
      : path{std::move(log_path)}, verbose{verbose_output} {}

  /// Records the candidate URI and exception message; verbose mode also writes to stderr.
  void record(std::string_view uri, const std::exception& error) const {
    std::ofstream file;
    file.exceptions(std::ios_base::failbit | std::ios_base::badbit);
    file.open(path, std::ofstream::out | std::ofstream::app);
    file << uri << ": " << error.what() << '\n';
    if (verbose) {
      std::cerr << "[-] Exception: " << error.what() << '\n';
    }
  }

private:
  std::string path;
  bool verbose;
};
} // namespace abrade
