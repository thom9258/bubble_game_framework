#pragma once

#include <bubble/internal/glm.hpp>
#include <bubble/resources.hpp>
#include <bubble/renderer.hpp>

#include <expected>
#include <string>
#include <vector>

namespace bubble {

struct ModelTreeMesh {
  MeshRef mesh;
  TextureRef texture; // tree-created texture or Resources::default_texture()
};

struct ModelTreeNode {
  std::string name;
  glm::mat4 transform{1.0f};          // local, relative to parent
  std::vector<ModelTreeMesh> meshes;  // handles: shared meshes/textures are the same ref
  std::vector<ModelTreeNode> children;
};

struct ModelTree {
  std::vector<ModelTreeNode> roots;

  // Everything this tree created, each exactly once, for destroy_model_tree().
  std::vector<MeshRef> created_meshes;
  std::vector<TextureRef> created_textures;
};

// Uploads every mesh and bitmap in the scene once and mirrors the node tree.
// On failure, everything created so far is destroyed again.
[[nodiscard]]
auto create_model_tree(GltfScene &scene) -> std::expected<ModelTree, std::string>;

auto destroy_model_tree(ModelTree &tree) -> void;

// Draws the whole tree with `transform` applied on top of the root transforms.
// Call between Renderer::begin_frame() and Renderer::end_frame().
auto draw_model_tree(const ModelTree &tree, const glm::mat4 &transform) -> void;

} // namespace bubble
