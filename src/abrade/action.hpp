#pragma once
#include <abrade/content_filter.hpp>
#include <abrade/exception.hpp>
#include <abrade/http_status.hpp>
#include <abrade/run_stats.hpp>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>

namespace abrade {

/// Handles HEAD responses by recording successful candidate paths.
///
/// A 2xx status is treated as discovered. Successful candidates are appended to
/// the configured output file; verbose mode also prints every observed status.
struct HeadAction {
  /// Opens the append-only output file used for discovered candidates.
  HeadAction(const std::string& output_path, bool verbose_output) : is_verbose{verbose_output} {
    boost::filesystem::path boost_path(output_path);
    boost::system::error_code ec;
    create_directories(boost_path.parent_path(), ec);
    file.exceptions(std::ios_base::failbit | std::ios_base::badbit);
    file.open(output_path, std::ofstream::out | std::ofstream::app);
  }

  /// Records a candidate when the response is successful and optionally logs status.
  void process(unsigned int status_code, const std::string_view& candidate) {
    if (is_success_status(status_code)) {
      file << candidate << '\n';
      file.flush();
    }
    if (is_verbose) {
      std::cout << candidate << ": " << status_code << '\n';
    }
  }

private:
  const bool is_verbose;
  std::ofstream file;
};

/// Handles GET responses by writing accepted response bodies to disk.
///
/// Only 2xx response bodies can be persisted. Verbose mode prints diagnostics
/// but deliberately does not change which bodies are written.
struct GetAction {
  /// Creates the output directory and configures body filters.
  GetAction(const std::string& output_dir, ContentFilters filters_in, bool verbose_output,
            RunStats& run_stats)
      : re{"[^a-zA-Z0-9.-]"}, is_verbose{verbose_output}, path_dir{output_dir},
        filters{std::move(filters_in)}, stats{run_stats} {
    boost::system::error_code ec;
    boost::filesystem::create_directories(output_dir, ec);
    if (ec) {
      throw AbradeException{"open path", ec};
    }
  }

  /// Writes successful accepted bodies using the candidate as a sanitized filename.
  void process(unsigned int status_code, std::string_view body, std::string_view candidate) {
    if (is_verbose) {
      std::cout << "[ ] Response body from " << candidate << ":\n" << body << '\n';
    }
    if (!is_success_status(status_code)) {
      return;
    }
    if (!filters.accepts(body)) {
      stats.record_filtered();
      return;
    }
    write_out(body, candidate);
  }

private:
  void write_out(std::string_view body, std::string_view candidate) {
    auto path{path_dir};
    path.append("/");
    path.append(regex_replace(std::string{candidate.begin(), candidate.end()}, re, "_"));
    std::ofstream file;
    file.exceptions(std::ios_base::failbit | std::ios_base::badbit);
    file.open(path, std::ofstream::out | std::ofstream::binary);
    file.write(body.data(), static_cast<std::streamsize>(body.size()));
    stats.record_bytes_written(body.size());
  }

  const boost::regex re;
  const bool is_verbose;
  const std::string path_dir;
  ContentFilters filters;
  RunStats& stats;
};
} // namespace abrade
