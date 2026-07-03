#pragma once
#include <string>
#include <fstream>
#include <boost/regex.hpp>
#include <boost/filesystem.hpp>
#include "Exception.h"
#include "HttpStatus.h"
#include <iostream>
#include <sstream>
#include <string_view>

/// Handles HEAD responses by recording successful candidate paths.
struct HeadAction {
  /// Opens the append-only output file used for discovered candidates.
  HeadAction(const std::string& output_path, bool verbose_output) : is_verbose {verbose_output} {
    boost::filesystem::path boost_path(output_path);
    boost::system::error_code ec;
    create_directories(boost_path.parent_path(), ec);
    file.exceptions(std::ios_base::failbit | std::ios_base::badbit);
    file.open(output_path, std::ofstream::out | std::ofstream::app);
  }

  /// Records a candidate when the response is successful and optionally logs status.
  void process(unsigned int status_code, const std::string_view& candidate) {
    if (is_success_status(status_code)) { file << candidate << std::endl; }
    if (is_verbose) std::cout << candidate << ": " << status_code << std::endl;
  }

private:
  const bool is_verbose;
  std::ofstream file;
};

/// Handles GET responses by writing successful response bodies to disk.
struct GetAction {
  /// Creates the output directory and configures optional body-screening text.
  GetAction(const std::string& output_dir, const std::string& screen_text, bool verbose_output)
    : re{ "[^a-zA-Z0-9.-]" }, is_verbose{ verbose_output }, path_dir{ output_dir }, screen{ screen_text } {
    boost::filesystem::path boost_path(output_dir);
    boost::system::error_code ec;
    boost::filesystem::create_directories(output_dir, ec);
    if (ec) throw AbradeException{"open path", ec};
  }

  /// Writes successful bodies, or every body in verbose mode, using the candidate as a filename.
  template <typename Stream>
  void process(unsigned int status_code, Stream contents, const std::string_view& candidate) {
    if (is_verbose) {
      std::stringstream ss;
      ss << contents;
      auto contents_string = ss.str();
      std::cout << "[ ] Response from " << candidate << ":\n" << contents_string << std::endl;
      write_out(contents_string, candidate);
    }
    else if (is_success_status(status_code)) { write_out(contents, candidate); }
  }

private:
  template <typename Stream>
  void write_out(Stream& contents, const std::string_view& candidate) {
    auto path{path_dir};
    path.append("/");
    path.append(regex_replace(std::string{ candidate.begin(), candidate.end() }, re, "_"));
    if (screen.empty()) {
      std::ofstream file;
      file.exceptions(std::ios_base::failbit | std::ios_base::badbit);
      file.open(path, std::ofstream::out);
      file << contents << std::endl;
    } else {
      std::stringstream ss;
      ss << contents << std::endl;
      const auto contents_str = ss.str();
      const auto contains_screen = contents_str.contains(screen);
      if (contains_screen) return;
      std::ofstream file;
      file.exceptions(std::ios_base::failbit | std::ios_base::badbit);
      file.open(path, std::ofstream::out);
      file << contents_str << std::endl;
    }
  }

  const boost::regex re;
  const bool is_verbose;
  const std::string path_dir, screen;
};
