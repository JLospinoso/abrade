#pragma once
#include <chrono>
#include <cstddef>
#include <iostream>
#include <boost/circular_buffer.hpp>
#include <numeric>
#include <algorithm>

/// Chooses how many scraper coroutines should be active after each completion.
struct Controller {
  virtual ~Controller() = default;
  /// Records a completed request and the current concurrency level.
  virtual void register_completion(size_t current_coroutines) = 0;
  /// Returns the desired number of active request coroutines.
  virtual size_t recommended_coroutines() const noexcept = 0;
};

/// Keeps the scraper at a fixed concurrency level.
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
struct AdaptiveController : Controller {
  explicit AdaptiveController(size_t initial_coroutines, size_t sample_size,
    size_t controller_sample_interval, size_t minimum_coroutines, size_t maximum_coroutines);

  void register_completion(size_t current_coroutines) override;

  size_t recommended_coroutines() const noexcept override;
private:
  void increase_recommendation() noexcept;

  boost::circular_buffer<size_t> coroutines;
  boost::circular_buffer<double> velocities;
  std::chrono::time_point<std::chrono::steady_clock> start;
  size_t completed, sample_interval, recommended, max_coro, min_coro;
};
