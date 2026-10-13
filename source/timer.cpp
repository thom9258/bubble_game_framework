#include <bubble/timer.hpp>

namespace bubble {

auto Timer::start() -> void { _start = clock_t::now(); }

auto Timer::stop() -> std::optional<duration_t> {
  if (!_start.has_value()) {
    return std::nullopt;
  }

  const auto now = clock_t::now();
  const auto duration = std::chrono::duration<double>(now - _start.value());
  _start = std::nullopt;
  return duration;
}

} // namespace bubble
