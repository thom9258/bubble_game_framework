#pragma once

#include "bubble/initialize_config.hpp"
#include <bubble/system_concept.hpp>
#include <bubble/bitmap.hpp>

#include <cstdint>
#include <optional>
#include <span>

namespace bubble {

struct Vertex {
  float position[3];
  float normal[3];
  float texture_coordinate[2];
};

using Index = std::uint32_t;

class MeshRef {
public:
  MeshRef(std::uint32_t index, std::uint32_t generation);
  auto index() const -> std::uint32_t;
  auto generation() const -> std::uint32_t;

private:
  std::uint32_t _index;
  std::uint32_t _generation;
};

class TextureRef {
public:
  TextureRef(std::uint32_t index, std::uint32_t generation);
  auto index() const -> std::uint32_t;
  auto generation() const -> std::uint32_t;

private:
  std::uint32_t _index;
  std::uint32_t _generation;
};

class Resources {
public:
  [[nodiscard]]
  static auto initialize(const InitializeConfig& config) -> bool;

  static auto destroy() -> void;

  [[nodiscard]]
  static auto create_mesh(std::span<Vertex> vertices, std::span<Index> indices)
      -> std::optional<MeshRef>;

  static auto destroy_mesh(MeshRef ref) -> void;

  [[nodiscard]]
  static auto create_texture(Bitmap &bitmap) -> std::optional<TextureRef>;

  static auto destroy_texture(TextureRef ref) -> void;

private:
  Resources() = delete;
};

} // namespace bubble

static_assert(bubble::System<bubble::Resources>);
