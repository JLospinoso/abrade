#pragma once

#include <chrono>
#include <cstddef>
#include <iomanip>
#include <sstream>
#include <string>

namespace abrade {

/// Aggregates one scraper invocation's observable runtime outcomes.
///
/// The scraper records request attempts and transport errors. Query objects record
/// HTTP status classes. Actions record filtered bodies and bytes persisted.
struct RunStats {
  /// Records one HTTP request attempt, including redirect follow-up requests.
  void record_attempt() noexcept { attempted_count++; }

  /// Records one completed HTTP response status.
  void record_response(unsigned int status_code) noexcept {
    if (status_code >= 200U && status_code < 300U) {
      success_count++;
    } else {
      non_success_count++;
    }
  }

  /// Records a successful response body omitted by configured content filters.
  void record_filtered() noexcept { filtered_count++; }

  /// Records a transport or runtime error for a candidate.
  void record_error() noexcept { error_count++; }

  /// Records response-body bytes actually written to disk.
  void record_bytes_written(std::size_t bytes) noexcept { bytes_written_count += bytes; }

  [[nodiscard]] std::size_t attempted() const noexcept { return attempted_count; }
  [[nodiscard]] std::size_t success_2xx() const noexcept { return success_count; }
  [[nodiscard]] std::size_t non_2xx() const noexcept { return non_success_count; }
  [[nodiscard]] std::size_t filtered() const noexcept { return filtered_count; }
  [[nodiscard]] std::size_t errors() const noexcept { return error_count; }
  [[nodiscard]] std::size_t bytes_written() const noexcept { return bytes_written_count; }
  [[nodiscard]] bool has_errors() const noexcept { return error_count != 0U; }

  /// Returns elapsed wall-clock seconds since this collector was constructed.
  [[nodiscard]] double elapsed_seconds() const {
    const auto elapsed = std::chrono::steady_clock::now() - started_at;
    return std::chrono::duration<double>{elapsed}.count();
  }

  /// Formats the final user-facing run summary.
  [[nodiscard]] std::string summary() const {
    const auto elapsed = elapsed_seconds();
    const auto requests_per_second =
        elapsed > 0.0 ? static_cast<double>(attempted_count) / elapsed : 0.0;
    const auto mib_per_second =
        elapsed > 0.0 ? static_cast<double>(bytes_written_count) / (1024.0 * 1024.0) / elapsed
                      : 0.0;

    std::ostringstream out;
    out << std::fixed << std::setprecision(2);
    out << "[ ] Summary: attempted=" << attempted_count << " 2xx=" << success_count
        << " non-2xx=" << non_success_count << " filtered=" << filtered_count
        << " errors=" << error_count << " bytes-written=" << bytes_written_count
        << " elapsed=" << elapsed << "s"
        << " requests/sec=" << requests_per_second << " MiB/sec=" << mib_per_second;
    return out.str();
  }

private:
  std::chrono::steady_clock::time_point started_at{std::chrono::steady_clock::now()};
  std::size_t attempted_count{};
  std::size_t success_count{};
  std::size_t non_success_count{};
  std::size_t filtered_count{};
  std::size_t error_count{};
  std::size_t bytes_written_count{};
};
} // namespace abrade
