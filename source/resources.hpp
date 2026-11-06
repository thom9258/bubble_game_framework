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
  auto get(TextureRef ref) -> TextureSource *;

  GenerationalArray<MeshSource, 100> mesh_sources;
  GenerationalArray<TextureSource, 100> texture_sources;

private:
  ResourcesImpl() = default;
};

} // namespace bubble
