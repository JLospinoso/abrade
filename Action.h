#pragma once
#include <string>
#include <fstream>
#include <boost/regex.hpp>
#include <boost/filesystem.hpp>
#include "Exception.h"
#include <iostream>
#include <sstream>

struct HeadAction {
  HeadAction(const std::string& path, bool is_verbose) : is_verbose {is_verbose} {
    boost::filesystem::path boost_path(path);
    boost::system::error_code ec;
    create_directories(boost_path.parent_path(), ec);
    file.exceptions(std::ios_base::failbit | std::ios_base::badbit);
    file.open(path, std::ofstream::out | std::ofstream::app);
  }

  void process(int status_code, const std::string& target) {
    if (status_code >= 200 && status_code < 300) { file << target << std::endl; }
    if (is_verbose) std::cout << target << ": " << status_code << std::endl;
  }

private:
  const bool is_verbose;
  std::ofstream file;
};

struct GetAction {
  GetAction(const std::string& path_dir, const std::string& screen, bool is_verbose)
    : re{ "[^a-zA-Z0-9.-]" }, is_verbose{ is_verbose }, path_dir{ path_dir }, screen{ screen } {
    boost::filesystem::path boost_path(path_dir);
    boost::system::error_code ec;
    boost::filesystem::create_directories(path_dir, ec);
    if (ec) throw AbradeException{"open path", ec};
  }

  template <typename Stream>
  void process(int status_code, Stream contents, const std::string& target) {
    if (is_verbose) {
      std::stringstream ss;
      ss << contents;
      auto contents_string = ss.str();
      std::cout << "[ ] Response from " << target << ":\n" << contents_string << std::endl;
      write_out(contents_string, target);
    }
    else if (status_code >= 200 && status_code < 300) { write_out(contents, target); }
  }

private:
  template <typename Stream>
  void write_out(Stream& contents, const std::string& target) {
    auto path{path_dir};
    path.append("/");
    path.append(regex_replace(target, re, "_"));
    if (screen.empty()) {
      std::ofstream file;
      file.exceptions(std::ios_base::failbit | std::ios_base::badbit);
      file.open(path, std::ofstream::out);
      file << contents << std::endl;
    } else {
      std::stringstream ss;
      ss << contents << std::endl;
      const auto contents_str = ss.str();
      const auto contains_screen = contents_str.find(screen) != std::string::npos;
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
