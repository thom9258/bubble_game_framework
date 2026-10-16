#pragma once

#include "gpu_buffer.hpp"
#include "generational_array.hpp"
#include "texture.hpp"

namespace bubble::resources
{

struct MeshSource {
  GPUBuffer vertices;
  GPUBuffer indices;
  std::uint32_t indices_count;
};

struct TextureSource {
    Texture texture;
};

struct Instance {
GenerationalArray<MeshSource, 100> mesh_sources;
GenerationalArray<TextureSource, 100> texture_sources;
};

auto instance() -> Instance &;

[[nodiscard]]
auto initialize() -> bool;

auto destroy() -> void;

auto get(MeshRef ref) -> MeshSource*;

auto get(TextureRef ref) -> TextureSource*;

}
