#pragma once

#include <bubble/internal/glm.hpp>
#include <bubble/renderer.hpp>
#include <bubble/resources.hpp>

#include <expected>
#include <string>
#include <vector>

namespace bubble {

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

struct ModelTreeMesh {
  MeshRef mesh;
  TextureRef texture;
};

struct ModelTreeNode {
  std::string name;
  glm::mat4 transform{1.0f};
  std::vector<ModelTreeMesh> meshes;
  std::vector<ModelTreeNode> children;
};

struct ModelTree {
  std::vector<ModelTreeNode> roots;
  std::vector<MeshRef> unique_meshes;
  std::vector<TextureRef> unique_textures;
};

struct Animator {
    auto play(std::string_view name);

    std::vector<GltfAnimation> animations;
};

struct Model {
  GltfScene scene;
  ModelTree tree;
};

auto load_gltf_scene(std::filesystem::path path)
    -> std::expected<GltfScene, std::string>;

[[nodiscard]]
auto create_model_tree(GltfScene &scene)
    -> std::expected<ModelTree, std::string>;

auto destroy_model_tree(ModelTree &tree) -> void;

auto draw_model_tree(const ModelTree &tree, glm::mat4 transform) -> void;

auto load_model(std::filesystem::path path)
    -> std::expected<Model, std::string>;

auto create_animator(Model &model) -> std::expected<Animator, std::string>;

} // namespace bubble
