#include <bubble/model_tree.hpp>

#include <format>

namespace bubble {

namespace {

// `uploaded` is indexed like GltfScene::meshes.
auto convert_node(const GltfModel &model, const std::vector<ModelTreeMesh> &uploaded)
    -> ModelTreeNode {
  ModelTreeNode node;
  node.name = model.name;
  node.transform = model.transform;

  node.meshes.reserve(model.meshes.size());
  for (const auto index : model.meshes) {
    node.meshes.push_back(uploaded[index]); // copies the handles, not the mesh
  }

  node.children.reserve(model.children.size());
  for (const auto &child : model.children) {
    node.children.push_back(convert_node(child, uploaded));
  }
  return node;
}

auto draw_node(const ModelTreeNode &node, const glm::mat4 &parent) -> void {
  const glm::mat4 world = parent * node.transform;

  for (const auto &m : node.meshes) {
    Renderer::draw_textured(world, m.mesh, m.texture);
  }
  for (const auto &child : node.children) {
    draw_node(child, world);
  }
}

} // namespace

auto create_model_tree(GltfScene &scene) -> std::expected<ModelTree, std::string> {
  ModelTree tree;

  auto fail = [&tree](std::string message) -> std::unexpected<std::string> {
    destroy_model_tree(tree); // release whatever was already uploaded
    return std::unexpected(std::move(message));
  };

  // --- Textures: one per bitmap (indexed like GltfScene::bitmaps) ----------
  tree.created_textures.reserve(scene.bitmaps.size());
  for (std::size_t i = 0; i < scene.bitmaps.size(); ++i) {
    auto texture = Resources::create_texture(scene.bitmaps[i]);
    if (!texture) {
      return fail(std::format("failed to create texture for bitmap {}", i));
    }
    tree.created_textures.push_back(*texture);
  }

  // --- Meshes: one per GltfMesh (indexed like GltfScene::meshes) -----------
  // Skinned meshes are uploaded as static meshes for now (bind pose).
  std::vector<ModelTreeMesh> uploaded;
  uploaded.reserve(scene.meshes.size());
  tree.created_meshes.reserve(scene.meshes.size());

  for (std::size_t i = 0; i < scene.meshes.size(); ++i) {
    const auto &mesh = scene.meshes[i];

    TextureRef texture = Resources::default_texture();
    if (mesh.material) {
      if (*mesh.material >= scene.materials.size()) {
        return fail(std::format("mesh {}: material index {} out of range", i, *mesh.material));
      }
      if (const auto diffuse = scene.materials[*mesh.material].diffuse) {
        if (*diffuse >= tree.created_textures.size()) {
          return fail(std::format("mesh {}: bitmap index {} out of range", i, *diffuse));
        }
        texture = tree.created_textures[*diffuse];
      }
    }

    auto ref = Resources::create_mesh(mesh.vertices, mesh.indices);
    if (!ref) {
      return fail(std::format("failed to create mesh {}", i));
    }
    tree.created_meshes.push_back(*ref);
    uploaded.push_back({*ref, texture});
  }

  // --- Node tree: indices resolved to handles -------------------------------
  tree.roots.reserve(scene.roots.size());
  for (const auto &root : scene.roots) {
    tree.roots.push_back(convert_node(root, uploaded));
  }

  return tree;
}

auto destroy_model_tree(ModelTree &tree) -> void {
  for (const auto &mesh : tree.created_meshes) {
    Resources::destroy_mesh(mesh);
  }
  for (const auto &texture : tree.created_textures) {
    Resources::destroy_texture(texture);
  }
  tree = {};
}

auto draw_model_tree(const ModelTree &tree, const glm::mat4 &transform) -> void {
  for (const auto &root : tree.roots) {
    draw_node(root, transform);
  }
}

} // namespace bubble
