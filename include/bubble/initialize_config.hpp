#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace bubble {

enum class LogType { info, warn, error };
using LogCallback = void (*)(LogType, std::string_view);

struct InitializeConfig {
  std::int32_t width{480};
  std::int32_t height{320};
  std::string name{"bubble"};

  struct {
    std::optional<LogCallback> log_callback;
    bool enable_graphics_validation{true};
    bool enable_vsync{true};
  } options;
};

} // namespace bubble
