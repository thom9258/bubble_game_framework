#pragma once

#include <bubble/system_concept.hpp>

#include <filesystem>
#include <optional>

namespace bubble {

class Context {
public:
  [[nodiscard]]
  static auto initialize(const InitializeConfig &config) -> bool;

  static auto destroy() -> void;

  static auto begin_frame() -> void;

  [[nodiscard]]
  static auto requested_close() -> bool;

  [[nodiscard]]
  static auto program_path() -> std::optional<std::filesystem::path>;

private:
  Context() = delete;
};

} // namespace bubble

static_assert(bubble::System<bubble::Context>);
static_assert(bubble::SystemHasBeginFrame<bubble::Context>);
