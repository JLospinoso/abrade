#pragma once
#include <exception>
#include <string>
#include <boost/system/system_error.hpp>

struct AbradareException : std::runtime_error {
  AbradareException(const std::string& msg);
  AbradareException(const std::string& msg, const boost::system::error_code& ec);
};
