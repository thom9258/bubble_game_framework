#pragma once

#include <bubble/system_concept.hpp>

namespace bubble {

class Context {
public:
  [[nodiscard]]
  static auto initialize(const InitializeConfig &config) -> bool;

  static auto destroy() -> void;

  static auto begin_frame() -> void;

  [[nodiscard]]
  static auto requested_close() -> bool;

private:
  Context() = delete;
};

} // namespace bubble

static_assert(bubble::System<bubble::Context>);
static_assert(bubble::SystemHasBeginFrame<bubble::Context>);
