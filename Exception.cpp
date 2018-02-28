#include "Exception.h"

AbradeException::AbradeException(const std::string& msg) : runtime_error{msg} { }

AbradeException::AbradeException(const std::string& msg, const boost::system::error_code& ec)
  : runtime_error{msg + "; Code " + std::to_string(ec.value()) + "; Message " + ec.message()} { }
