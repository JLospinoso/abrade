#pragma once

#include <exception>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include "Candidate.h"

inline Candidate make_candidate(std::string uri) {
  return Candidate{
    std::move(uri),
    {},
    {}
  };
}

struct FileErrorLog {
  FileErrorLog(std::string path, bool verbose)
    : path{std::move(path)},
      verbose{verbose} { }

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
