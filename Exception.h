#pragma once
#include <exception>
#include <string>
#include <boost/system/system_error.hpp>

struct AbradeException : std::runtime_error {
  AbradeException(const std::string& msg);
  AbradeException(const std::string& msg, const boost::system::error_code& ec);
};
