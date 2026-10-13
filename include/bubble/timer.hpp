#pragma once

#include <chrono>

namespace bubble {

class Timer {
public:
  using clock_t = std::chrono::high_resolution_clock;
  using time_point_t = std::chrono::time_point<clock_t>;
  using duration_t = std::chrono::duration<double>;

  auto start() -> void;

  [[nodiscard]]
  auto stop() -> std::optional<duration_t>;

private:
  std::optional<time_point_t> _start;
};

} // namespace bubble
