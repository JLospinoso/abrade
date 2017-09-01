#include "Controller.h"

using namespace std;

FixedController::FixedController(size_t coroutines, size_t sampling_interval) :
  coroutines{ coroutines },
  sampling_interval{ sampling_interval },
  completed{} { }

void FixedController::register_completion(unsigned int current_coroutines) {
  if (++completed < sampling_interval) return;
  const auto end = chrono::system_clock::now();
  const chrono::duration<double> elapsed = end - start;
  const auto velocity = static_cast<double>(completed) / static_cast<double>(elapsed.count());
  cout << "[ ] Request velocity: " << velocity << " rps. Recommended coros (fixed): " << coroutines << "; Current coros: " << current_coroutines << endl;
  start = end;
  completed = 0;
}

size_t FixedController::recommended_coroutines() const noexcept {
  return coroutines;
}

AdaptiveController::AdaptiveController(size_t initial_coroutines, size_t sample_size,
  size_t sample_interval, size_t minimum_coroutines, size_t maximum_coroutines) :
  coroutines{sample_size},
  velocities{sample_size},
  start{chrono::system_clock::now()},
  completed{},
  sample_interval{sample_interval},
  recommended{initial_coroutines},
  max_coro{ maximum_coroutines },
  min_coro{ minimum_coroutines } { }

void AdaptiveController::register_completion(unsigned int current_coroutines) {
  if (++completed < sample_interval) return;
  const auto end = chrono::system_clock::now();
  const chrono::duration<double> elapsed = end - start;
  velocities.push_back(static_cast<double>(completed) / static_cast<double>(elapsed.count()));
  coroutines.push_back(current_coroutines);
  cout << "[ ] Request velocity: " << velocities.back() << " rps. Concurrent requests: " << coroutines.back() << endl;
  start = end;
  completed = 0;

  if (velocities.size() < 2) {
    recommended++;
    return; // Insufficient samples
  }
  // Fit regression:
  const auto sum_velocity = accumulate(velocities.begin(), velocities.end(), double{ 0 });
  const auto mean_velocity = static_cast<double>(sum_velocity) / static_cast<double>(velocities.size());
  const auto sum_coros = accumulate(coroutines.begin(), coroutines.end(), 0);
  const auto mean_coros = static_cast<double>(sum_coros) / static_cast<double>(coroutines.size());
  double ss_coros{};
  for (const auto coro : coroutines) {
    const auto delta = coro - mean_coros;
    ss_coros += delta * delta;
  }
  if (ss_coros < 0.0001) {
    recommended++;
    return; // Insufficient variation
  }
  double ss_covar{};
  for (int i{}; i < velocities.size(); i++) {
    ss_covar += (coroutines[i] - mean_coros) * (velocities[i] - mean_velocity);
  }
  if (ss_covar < 0.0001) {
    recommended++;
    return; // Insufficient variation
  }
  const auto beta = ss_coros / ss_covar;
  if (beta > 0 && recommended < max_coro) {
    recommended++;
  } else if (beta < 0 && recommended > min_coro) {
    recommended--;
  }
}

size_t AdaptiveController::recommended_coroutines() const noexcept {
  return recommended;
}
