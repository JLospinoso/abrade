#pragma once
#include <algorithm>
#include <boost/circular_buffer.hpp>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <numeric>

namespace abrade {

/// Chooses how many scraper coroutines should be active after each completion.
///
/// The scraper consults the controller after every completed candidate. A
/// controller may keep concurrency fixed or adapt it based on observed
/// throughput.
struct Controller {
  Controller() = default;
  Controller(const Controller&) = default;
  Controller(Controller&&) = default;
  Controller& operator=(const Controller&) = default;
  Controller& operator=(Controller&&) = default;
  virtual ~Controller() = default;
  /// Records a completed request and the current concurrency level.
  virtual void register_completion(size_t current_coroutines) = 0;
  /// Returns the desired number of active request coroutines.
  virtual size_t recommended_coroutines() const noexcept = 0;
};

/// Keeps the scraper at a fixed concurrency level.
///
/// Completion counts are sampled only for progress diagnostics; the recommended
/// coroutine count never changes.
struct FixedController : Controller {
  explicit FixedController(size_t fixed_coroutines, size_t fixed_sampling_interval);

  void register_completion(size_t current_coroutines) override;

  size_t recommended_coroutines() const noexcept override;

private:
  std::chrono::time_point<std::chrono::steady_clock> start;
  const size_t coroutines, sampling_interval;
  size_t completed;
};

/// Adjusts concurrency using a simple velocity/concurrency trend estimate.
///
/// The controller records completion velocity over a sliding window and nudges
/// the recommendation within the configured minimum and maximum bounds.
struct AdaptiveController : Controller {
  explicit AdaptiveController(size_t initial_coroutines, size_t sample_size,
                              size_t controller_sample_interval, size_t minimum_coroutines,
                              size_t maximum_coroutines);

  void register_completion(size_t current_coroutines) override;

  size_t recommended_coroutines() const noexcept override;

private:
  void increase_recommendation() noexcept;

  boost::circular_buffer<size_t> coroutines;
  boost::circular_buffer<double> velocities;
  std::chrono::time_point<std::chrono::steady_clock> start;
  size_t completed, sample_interval, recommended, max_coro, min_coro;
};
} // namespace abrade
