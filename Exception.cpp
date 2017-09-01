#include "Exception.h"

AbradareException::AbradareException(const std::string& msg) : runtime_error{msg} { }

AbradareException::AbradareException(const std::string& msg, const boost::system::error_code& ec)
  : runtime_error{msg + "; Code " + std::to_string(ec.value()) + "; Message " + ec.message()} { }
