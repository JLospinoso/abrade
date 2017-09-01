#pragma once
#include <chrono>
#include <iostream>
#include <boost/circular_buffer.hpp>
#include <numeric>
#include <algorithm>

struct Controller {
  virtual ~Controller() { }
  virtual void register_completion(unsigned int current_coroutines) = 0;
  virtual size_t recommended_coroutines() const noexcept = 0;
};

struct FixedController : Controller {
  explicit FixedController(size_t coroutines, size_t sampling_interval);

  void register_completion(unsigned int current_coroutines) override;

  size_t recommended_coroutines() const noexcept override;
private:
  std::chrono::time_point<std::chrono::system_clock> start;
  const size_t coroutines, sampling_interval;
  size_t completed;
};


struct AdaptiveController : Controller {
  explicit AdaptiveController(size_t initial_coroutines, size_t sample_size,
    size_t sample_interval, size_t minimum_coroutines, size_t maximum_coroutines);

  void register_completion(unsigned int current_coroutines) override;

  size_t recommended_coroutines() const noexcept override;
private:
  boost::circular_buffer<unsigned int> coroutines;
  boost::circular_buffer<double> velocities;
  std::chrono::time_point<std::chrono::system_clock> start;
  size_t completed, sample_interval, recommended, max_coro, min_coro;
};
