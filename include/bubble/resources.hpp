#pragma once

#include <bubble/animation.hpp>
#include <bubble/bitmap.hpp>
#include <bubble/initialize_config.hpp>
#include <bubble/internal/glm.hpp>
#include <bubble/system_concept.hpp>

#include <cstdint>
#include <optional>
#include <span>
#include <unordered_map>

namespace bubble {

struct Vertex {
  float position[3];
  float normal[3];
  float texture_coordinate[2];
};

struct BoneVertex {
  int ids[4];
  float weights[4];
};

struct AnimatedVertex {
  float position[3];
  float normal[3];
  float texture_coordinate[2];
  int bone_ids[4];
  float bone_weights[4];
};

using Index = std::uint32_t;

struct GltfMesh {
  std::vector<Vertex> vertices;
  std::vector<BoneVertex> bone_vertices;
  std::vector<Index> indices;
  std::optional<std::size_t> material;
};

struct GltfMaterial {
  std::string name;
  std::optional<std::size_t> diffuse;
};

class GltfMetaData {
public:
  auto set_int(std::string_view name, std::int32_t value) -> bool;
  auto set_string(std::string_view name, const std::string &value) -> bool;
  auto set_float(std::string_view name, float value) -> bool;
  auto set_bool(std::string_view name, bool value) -> bool;

  auto get_int(std::string_view name) -> std::optional<std::int32_t>;
  auto get_string(std::string_view name) -> std::optional<std::string>;
  auto get_float(std::string_view name) -> std::optional<float>;
  auto get_bool(std::string_view name) -> std::optional<bool>;

private:
  std::unordered_map<std::string, std::int32_t> _ints;
  std::unordered_map<std::string, std::string> _strings;
  std::unordered_map<std::string, float> _floats;
  std::unordered_map<std::string, bool> _bools;
};

struct GltfModel {
  std::string name;
  glm::mat4 transform;
  std::vector<std::size_t> meshes;
  std::vector<GltfModel> children;
  GltfMetaData metadata;
};

struct GltfAnimation {
  std::string name;
  animation::Animation animation;
};

struct GltfScene {
  std::vector<GltfModel> roots;
  std::vector<GltfMesh> meshes;
  std::vector<GltfMaterial> materials;
  std::vector<Bitmap> bitmaps;
  std::vector<GltfAnimation> animations;
};

#if 0
struct FlattenedModel {
  glm::mat4 global_transform;
  Model *model;
};

auto flatten_gltf_scene(GltfScene &scene) -> std::vector<ModelRef> {
  if (!scene.root.has_value()) {
    return {};
  }

  std::vector<ModelRef> refs;
  const auto root_transform = glm::mat4(1.0f);
  flatten_model(refs, scene.root.value(), root_transform);
  return refs;
}
#endif

class MeshRef {
public:
  MeshRef(std::uint32_t index, std::uint32_t generation);
  auto index() const -> std::uint32_t;
  auto generation() const -> std::uint32_t;

private:
  std::uint32_t _index;
  std::uint32_t _generation;
};

class AnimatedMeshRef {
public:
  AnimatedMeshRef(std::uint32_t index, std::uint32_t generation);
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
  static auto initialize(const InitializeConfig &config) -> bool;

  static auto destroy() -> void;

  static auto load_gltf(std::filesystem::path path)
      -> std::expected<GltfScene, std::string>;

  [[nodiscard]]
  static auto create_mesh(std::span<const Vertex> vertices,
                          std::span<const Index> indices)
      -> std::optional<MeshRef>;

  static auto destroy_mesh(MeshRef ref) -> void;

  [[nodiscard]]
  static auto create_animated_mesh(std::span<const Vertex> vertices,
                                   std::span<const BoneVertex> bone_vertices,
                                   std::span<const Index> indices)
      -> std::optional<AnimatedMeshRef>;

  [[nodiscard]]
  static auto create_animated_mesh(std::span<const AnimatedVertex> vertices,
                                   std::span<const Index> indices)
      -> std::optional<AnimatedMeshRef>;

  static auto destroy_animated_mesh(AnimatedMeshRef ref) -> void;

  [[nodiscard]]
  static auto create_texture(Bitmap &bitmap) -> std::optional<TextureRef>;

  static auto destroy_texture(TextureRef ref) -> void;

  [[nodiscard]]
  static auto default_mesh() -> MeshRef;

  [[nodiscard]]
  static auto default_texture() -> TextureRef;

private:
  Resources() = delete;
};

} // namespace bubble

static_assert(bubble::System<bubble::Resources>);
