#pragma once
#include <boost/system/system_error.hpp>
#include <stdexcept>
#include <string>

namespace abrade {

/// Application exception that carries Abrade context and optional Boost system errors.
///
/// Networking and parser helpers throw this when the caller should see
/// product-level context rather than a raw library exception.
struct AbradeException : std::runtime_error {
  explicit AbradeException(const std::string& msg);
  AbradeException(const std::string& msg, const boost::system::error_code& ec);
};
} // namespace abrade
