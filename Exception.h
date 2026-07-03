#pragma once
#include <stdexcept>
#include <string>
#include <boost/system/system_error.hpp>

/// Application exception that carries either Abrade context or Boost system errors.
struct AbradeException : std::runtime_error {
  explicit AbradeException(const std::string& msg);
  AbradeException(const std::string& msg, const boost::system::error_code& ec);
};
