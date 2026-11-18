#pragma once

#include <bubble/resources.hpp>

#include "generational_array.hpp"
#include "gpu_buffer.hpp"
#include "texture.hpp"

namespace bubble {

struct MeshSource {
  GPUBuffer vertices;
  GPUBuffer indices;
  std::uint32_t indices_count;
};

struct AnimatedMeshSource {
  GPUBuffer vertices;
  GPUBuffer indices;
  std::uint32_t indices_count;
};

struct TextureSource {
  Texture texture;
};

class ResourcesImpl {
public:
  friend Resources;
  static auto instance() -> ResourcesImpl &;

  [[nodiscard]]
  auto get(MeshRef ref) -> MeshSource *;

  [[nodiscard]]
  auto get(AnimatedMeshRef ref) -> AnimatedMeshSource *;

  [[nodiscard]]
  auto get(TextureRef ref) -> TextureSource *;

  GenerationalArray<MeshSource, 100> mesh_sources;
  GenerationalArray<AnimatedMeshSource, 100> animated_mesh_sources;
  GenerationalArray<TextureSource, 100> texture_sources;
  std::optional<MeshRef> default_mesh;
  std::optional<TextureRef> default_texture;

private:
  ResourcesImpl() = default;
};

} // namespace bubble
