#pragma once

/// Returns true when an HTTP status code is in the 2xx success class.
[[nodiscard]] inline bool is_success_status(unsigned int status_code) noexcept {
  return status_code >= 200U && status_code < 300U;
}
