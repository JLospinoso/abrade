#pragma once

inline bool is_success_status(int status_code) noexcept {
  return status_code >= 200 && status_code < 300;
}
