#include "resources.hpp"

#include <bubble/bubble.hpp>
#include <expected>
#include <filesystem>

#include "bubble/renderer.hpp"
#include "bubble/resources.hpp"
#include "context.hpp"
#include "direct_buffer.hpp"
#include "renderer.hpp"
#include "stb.hpp"

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wcast-qual"
#endif

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <memory>
#include <span>
#include <unordered_set>

namespace bubble {

static std::unique_ptr<ResourcesImpl> m_instance{nullptr};

auto ResourcesImpl::instance() -> ResourcesImpl & { return *m_instance; }

auto cube_vertices() -> std::span<const Vertex> {
  static constexpr auto vertices = std::array{
      Vertex{{-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
      Vertex{{0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
      Vertex{{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
      Vertex{{-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},

      Vertex{{0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},
      Vertex{{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},
      Vertex{{-0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},
      Vertex{{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},

      Vertex{{-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
      Vertex{{-0.5f, -0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
      Vertex{{-0.5f, 0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
      Vertex{{-0.5f, 0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},

      Vertex{{0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
      Vertex{{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
      Vertex{{0.5f, 0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
      Vertex{{0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},

      Vertex{{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
      Vertex{{0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
      Vertex{{0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
      Vertex{{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},

      Vertex{{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
      Vertex{{0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
      Vertex{{0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
      Vertex{{-0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}}};

  return vertices;
}

auto cube_indices() -> std::span<const Index> {
  static constexpr auto indices =
      std::array{0u,  1u,  2u,  2u,  3u,  0u,  4u,  5u,  6u,  6u,  7u,  4u,
                 8u,  9u,  10u, 10u, 11u, 8u,  12u, 13u, 14u, 14u, 15u, 12u,
                 16u, 17u, 18u, 18u, 19u, 16u, 20u, 21u, 22u, 22u, 23u, 20u};
  return indices;
}

auto checkerboard_bitmap() -> Bitmap {

  auto bitmap = bubble::Bitmap(64, 64, bubble::Color(255, 255, 0, 255));
  for (std::uint32_t i = 0; i < 32; i++) {
    for (std::uint32_t j = 0; j < 32; j++) {
      *bitmap.at(i, j) = bubble::color::red;
    }
  }

  for (std::uint32_t i = 32; i < 64; i++) {
    for (std::uint32_t j = 32; j < 64; j++) {
      *bitmap.at(i, j) = bubble::color::red;
    }
  }

  return bitmap;
}

auto initialize_defaults() -> bool {

  ResourcesImpl::instance().default_mesh =
      Resources::create_mesh(cube_vertices(), cube_indices());

  auto bitmap = checkerboard_bitmap();
  ResourcesImpl::instance().default_texture = Resources::create_texture(bitmap);
  return true;
}

[[nodiscard]]
auto Resources::initialize(const InitializeConfig &) -> bool {
  if (m_instance != nullptr) {
    destroy();
  }

  m_instance.reset(new ResourcesImpl);

  if (!initialize_defaults()) {
    return false;
  }

  return true;
}

auto Resources::destroy() -> void { m_instance.reset(); }

auto ResourcesImpl::get(MeshRef ref) -> MeshSource * {
  return mesh_sources.get(Ref(ref.index(), ref.generation()));
}

auto ResourcesImpl::get(AnimatedMeshRef ref) -> AnimatedMeshSource * {
  return animated_mesh_sources.get(Ref(ref.index(), ref.generation()));
}

auto ResourcesImpl::get(TextureRef ref) -> TextureSource * {
  return texture_sources.get(Ref(ref.index(), ref.generation()));
}

MeshRef::MeshRef(std::uint32_t index, std::uint32_t generation)
    : _index{index}, _generation{generation} {}

auto MeshRef::index() const -> std::uint32_t { return _index; }

auto MeshRef::generation() const -> std::uint32_t { return _generation; }

AnimatedMeshRef::AnimatedMeshRef(std::uint32_t index, std::uint32_t generation)
    : _index{index}, _generation{generation} {}

auto AnimatedMeshRef::index() const -> std::uint32_t { return _index; }

auto AnimatedMeshRef::generation() const -> std::uint32_t {
  return _generation;
}

TextureRef::TextureRef(std::uint32_t index, std::uint32_t generation)
    : _index{index}, _generation{generation} {}

auto TextureRef::index() const -> std::uint32_t { return _index; }

auto TextureRef::generation() const -> std::uint32_t { return _generation; }

auto Resources::create_mesh(std::span<const Vertex> vertices,
                            std::span<const Index> indices)
    -> std::optional<MeshRef> {
  const auto vertices_stager_usage = vk::BufferUsageFlagBits::eTransferSrc |
                                     vk::BufferUsageFlagBits::eTransferDst;
  const auto vertices_allocation_size = sizeof(vertices[0]) * vertices.size();
  auto vertices_stager =
      DirectBuffer(vertices_stager_usage, vertices_allocation_size);
  std::memcpy(vertices_stager.ptr(), &vertices[0], vertices_allocation_size);

  const auto indices_stager_usage = vk::BufferUsageFlagBits::eTransferSrc |
                                    vk::BufferUsageFlagBits::eTransferDst;
  const auto indices_allocation_size = sizeof(indices[0]) * indices.size();
  auto indices_stager =
      DirectBuffer(indices_stager_usage, indices_allocation_size);
  std::memcpy(indices_stager.ptr(), &indices[0], indices_allocation_size);

  const auto vertices_buffer_usage = vk::BufferUsageFlagBits::eVertexBuffer |
                                     vk::BufferUsageFlagBits::eTransferDst;
  auto vertices_buffer =
      GPUBuffer(vertices_buffer_usage, vertices_allocation_size);

  const auto indices_buffer_usage = vk::BufferUsageFlagBits::eIndexBuffer |
                                    vk::BufferUsageFlagBits::eTransferDst;
  auto indices_buffer =
      GPUBuffer(indices_buffer_usage, indices_allocation_size);

  vk::Result result =
      ContextImpl::immediate_evaluate([&](vk::CommandBuffer commandbuffer) {
        auto vertices_copy =
            vk::BufferCopy{}.setSrcOffset(0).setDstOffset(0).setSize(
                vertices_allocation_size);

        commandbuffer.copyBuffer(vertices_stager.buffer(),
                                 vertices_buffer.buffer(), {vertices_copy});

        auto indices_copy =
            vk::BufferCopy{}.setSrcOffset(0).setDstOffset(0).setSize(
                indices_allocation_size);

        commandbuffer.copyBuffer(indices_stager.buffer(),
                                 indices_buffer.buffer(), {indices_copy});
      });

  if (result != vk::Result::eSuccess) {
    return std::nullopt;
  }

  auto ref = ResourcesImpl::instance().mesh_sources.add(
      MeshSource(std::move(vertices_buffer), std::move(indices_buffer),
                 static_cast<std::uint32_t>(indices.size())));

  if (!ref.has_value()) {
    return std::nullopt;
  }

  return MeshRef(ref->index(), ref->generation());
}

auto Resources::destroy_mesh(MeshRef ref) -> void {
  ResourcesImpl::instance().mesh_sources.remove(
      Ref(ref.index(), ref.generation()));
}

auto create_animated_vertex(Vertex vertex, BoneVertex bone_vertex)
    -> AnimatedVertex {
  auto out = AnimatedVertex{};
  std::memcpy(out.position, vertex.position, sizeof(out.position));
  std::memcpy(out.normal, vertex.normal, sizeof(out.normal));
  std::memcpy(out.texture_coordinate, vertex.texture_coordinate,
              sizeof(out.texture_coordinate));
  std::memcpy(out.bone_ids, bone_vertex.ids, sizeof(out.bone_ids));
  std::memcpy(out.bone_weights, bone_vertex.weights, sizeof(out.bone_weights));
  return out;
}

[[nodiscard]]
auto Resources::create_animated_mesh(std::span<const Vertex> vertices,
                                     std::span<const BoneVertex> bone_vertices,
                                     std::span<const Index> indices)
    -> std::optional<AnimatedMeshRef> {
  auto animated_vertices = std::vector<bubble::AnimatedVertex>(vertices.size());
  for (std::size_t i = 0; i < vertices.size(); ++i) {
    animated_vertices[i] =
        create_animated_vertex(vertices[i], bone_vertices[i]);
  }

  return create_animated_mesh(animated_vertices, indices);
}

auto Resources::create_animated_mesh(std::span<const AnimatedVertex> vertices,
                                     std::span<const Index> indices)
    -> std::optional<AnimatedMeshRef> {
  const auto vertices_stager_usage = vk::BufferUsageFlagBits::eTransferSrc |
                                     vk::BufferUsageFlagBits::eTransferDst;
  const auto vertices_allocation_size = sizeof(vertices[0]) * vertices.size();
  auto vertices_stager =
      DirectBuffer(vertices_stager_usage, vertices_allocation_size);
  std::memcpy(vertices_stager.ptr(), &vertices[0], vertices_allocation_size);

  const auto indices_stager_usage = vk::BufferUsageFlagBits::eTransferSrc |
                                    vk::BufferUsageFlagBits::eTransferDst;
  const auto indices_allocation_size = sizeof(indices[0]) * indices.size();
  auto indices_stager =
      DirectBuffer(indices_stager_usage, indices_allocation_size);
  std::memcpy(indices_stager.ptr(), &indices[0], indices_allocation_size);

  const auto vertices_buffer_usage = vk::BufferUsageFlagBits::eVertexBuffer |
                                     vk::BufferUsageFlagBits::eTransferDst;
  auto vertices_buffer =
      GPUBuffer(vertices_buffer_usage, vertices_allocation_size);

  const auto indices_buffer_usage = vk::BufferUsageFlagBits::eIndexBuffer |
                                    vk::BufferUsageFlagBits::eTransferDst;
  auto indices_buffer =
      GPUBuffer(indices_buffer_usage, indices_allocation_size);

  vk::Result result =
      ContextImpl::immediate_evaluate([&](vk::CommandBuffer commandbuffer) {
        auto vertices_copy =
            vk::BufferCopy{}.setSrcOffset(0).setDstOffset(0).setSize(
                vertices_allocation_size);

        commandbuffer.copyBuffer(vertices_stager.buffer(),
                                 vertices_buffer.buffer(), {vertices_copy});

        auto indices_copy =
            vk::BufferCopy{}.setSrcOffset(0).setDstOffset(0).setSize(
                indices_allocation_size);

        commandbuffer.copyBuffer(indices_stager.buffer(),
                                 indices_buffer.buffer(), {indices_copy});
      });

  if (result != vk::Result::eSuccess) {
    return std::nullopt;
  }

  auto ref = ResourcesImpl::instance().animated_mesh_sources.add(
      AnimatedMeshSource(std::move(vertices_buffer), std::move(indices_buffer),
                         static_cast<std::uint32_t>(indices.size())));

  if (!ref.has_value()) {
    return std::nullopt;
  }

  return AnimatedMeshRef(ref->index(), ref->generation());
}

auto Resources::destroy_animated_mesh(AnimatedMeshRef ref) -> void {
  ResourcesImpl::instance().animated_mesh_sources.remove(
      Ref(ref.index(), ref.generation()));
}

auto Resources::create_texture(Bitmap &bitmap) -> std::optional<TextureRef> {

  const auto stager_usage = vk::BufferUsageFlagBits::eTransferSrc |
                            vk::BufferUsageFlagBits::eTransferDst;
  auto stager = DirectBuffer(stager_usage, bitmap.memory_size());
  std::memcpy(stager.ptr(), bitmap.pixels(), bitmap.memory_size());

  const auto extent =
      vk::Extent2D{}.setWidth(bitmap.width()).setHeight(bitmap.height());

  const auto usage =
      vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst;

  auto texture = Texture(extent, usage, vk::Format::eR8G8B8A8Srgb,
                         vk::ImageAspectFlagBits::eColor);

  vk::Result result =
      ContextImpl::immediate_evaluate([&](vk::CommandBuffer commandbuffer) {
        // Transition image to color override
        {
          auto range = vk::ImageSubresourceRange{}
                           .setAspectMask(vk::ImageAspectFlagBits::eColor)
                           .setBaseMipLevel(0)
                           .setLevelCount(1)
                           .setBaseArrayLayer(0)
                           .setLayerCount(1);

          auto barrier =
              vk::ImageMemoryBarrier{}
                  .setOldLayout(vk::ImageLayout::eUndefined)
                  .setNewLayout(vk::ImageLayout::eTransferDstOptimal)
                  .setImage(texture.image())
                  .setSubresourceRange(range)
                  .setSrcAccessMask(vk::AccessFlags())
                  .setDstAccessMask(vk::AccessFlagBits::eTransferWrite);

          commandbuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe,
                                        vk::PipelineStageFlagBits::eTransfer,
                                        vk::DependencyFlags(), nullptr, nullptr,
                                        barrier);
        }

        // Copy buffer into image
        {
          auto layer = vk::ImageSubresourceLayers{}
                           .setAspectMask(vk::ImageAspectFlagBits::eColor)
                           .setMipLevel(0)
                           .setBaseArrayLayer(0)
                           .setLayerCount(1);

          const auto offset = vk::Offset3D{}.setX(0).setY(0).setZ(0);

          const auto extent = vk::Extent3D{}
                                  .setWidth(bitmap.width())
                                  .setHeight(bitmap.height())
                                  .setDepth(1);

          auto region = vk::BufferImageCopy{}
                            .setBufferOffset(0)
                            .setBufferRowLength(0)
                            .setBufferImageHeight(0)
                            .setImageSubresource(layer)
                            .setImageOffset(offset)
                            .setImageExtent(extent);

          commandbuffer.copyBufferToImage(stager.buffer(), texture.image(),
                                          vk::ImageLayout::eTransferDstOptimal,
                                          region);
        }

        // Transfer image to shader readonly optimal
        {
          const auto source_range =
              vk::ImageSubresourceRange{}
                  .setAspectMask(vk::ImageAspectFlagBits::eColor)
                  .setBaseMipLevel(0)
                  .setLevelCount(1)
                  .setBaseArrayLayer(0)
                  .setLayerCount(1);

          auto barrier =
              vk::ImageMemoryBarrier{}
                  .setOldLayout(vk::ImageLayout::eTransferDstOptimal)
                  .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
                  .setImage(texture.image())
                  .setSubresourceRange(source_range)
                  .setSrcAccessMask(vk::AccessFlags())
                  .setDstAccessMask(vk::AccessFlagBits::eTransferWrite);

          commandbuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe,
                                        vk::PipelineStageFlagBits::eTransfer,
                                        vk::DependencyFlags(), nullptr, nullptr,
                                        barrier);
        }
      });

  if (result != vk::Result::eSuccess) {
    return std::nullopt;
  }

  auto ref = ResourcesImpl::instance().texture_sources.add(
      TextureSource(std::move(texture)));

  if (!ref.has_value()) {
    return std::nullopt;
  }

  return TextureRef(ref->index(), ref->generation());
}

auto Resources::destroy_texture(TextureRef ref) -> void {
  ResourcesImpl::instance().texture_sources.remove(
      Ref(ref.index(), ref.generation()));
}

auto Resources::default_mesh() -> MeshRef {
  return ResourcesImpl::instance().default_mesh.value();
}

auto Resources::default_texture() -> TextureRef {
  return ResourcesImpl::instance().default_texture.value();
}

auto maybe_insert(auto &map, std::string_view name, auto &&value) -> bool {
  if (map.contains(std::string(name))) {
    return false;
  }

  map.emplace(std::make_pair(name, value));
  return true;
}

auto GltfMetaData::set_int(std::string_view name, std::int32_t value) -> bool {
  return maybe_insert(_ints, name, value);
}

auto GltfMetaData::set_string(std::string_view name, const std::string &value)
    -> bool {
  return maybe_insert(_strings, name, value);
}

auto GltfMetaData::set_float(std::string_view name, float value) -> bool {
  return maybe_insert(_floats, name, value);
}

auto GltfMetaData::set_bool(std::string_view name, bool value) -> bool {
  return maybe_insert(_bools, name, value);
}

template <typename T>
auto maybe_get(auto &map, std::string_view name) -> std::optional<T> {
  auto found = map.find(std::string(name));
  if (found == map.end()) {
    return std::nullopt;
  }

  return found->second;
}

auto GltfMetaData::get_int(std::string_view name)
    -> std::optional<std::int32_t> {
  return maybe_get<std::int32_t>(_ints, name);
}

auto GltfMetaData::get_string(std::string_view name)
    -> std::optional<std::string> {
  return maybe_get<std::string>(_strings, name);
}

auto GltfMetaData::get_float(std::string_view name) -> std::optional<float> {
  return maybe_get<float>(_floats, name);
}

auto GltfMetaData::get_bool(std::string_view name) -> std::optional<bool> {
  return maybe_get<bool>(_bools, name);
}

struct CgltfDeleter {
  void operator()(cgltf_data *d) const noexcept { cgltf_free(d); }
};

using CgltfPtr = std::unique_ptr<cgltf_data, CgltfDeleter>;

struct StbDeleter {
  void operator()(stbi_uc *p) const noexcept { stbi_image_free(p); }
};

using StbPtr = std::unique_ptr<stbi_uc, StbDeleter>;

auto result_string(cgltf_result r) -> std::string_view {
  switch (r) {
  case cgltf_result_success:
    return "success";
  case cgltf_result_data_too_short:
    return "data too short";
  case cgltf_result_unknown_format:
    return "unknown format";
  case cgltf_result_invalid_json:
    return "invalid json";
  case cgltf_result_invalid_gltf:
    return "invalid gltf";
  case cgltf_result_invalid_options:
    return "invalid options";
  case cgltf_result_file_not_found:
    return "file not found";
  case cgltf_result_io_error:
    return "io error";
  case cgltf_result_out_of_memory:
    return "out of memory";
  case cgltf_result_legacy_gltf:
    return "legacy gltf (1.0) not supported";
  default:
    return "unknown error";
  }
}

static_assert(sizeof(bubble::Color) == 4 && alignof(bubble::Color) == 1,
              "Color must be 4 packed bytes to memcpy RGBA8 pixels into it");

auto make_bitmap(int width, int height, const stbi_uc *rgba) -> bubble::Bitmap {
  const auto w = static_cast<std::uint32_t>(width);
  const auto h = static_cast<std::uint32_t>(height);
  bubble::Bitmap bmp(w, h);
  std::memcpy(bmp.pixels(), rgba,
              static_cast<std::size_t>(w) * h * sizeof(bubble::Color));
  return bmp;
}

// ---------------------------------------------------------------------------
// Images: .glb buffer view, base64 data URI, or external file
// ---------------------------------------------------------------------------
auto load_image(const cgltf_image &img, const std::filesystem::path &base_dir)
    -> std::expected<bubble::Bitmap, std::string> {
  int w = 0, h = 0, c = 0;
  StbPtr px;

  if (img.buffer_view) {
    const auto *bv = img.buffer_view;
    const auto *bytes =
        static_cast<const stbi_uc *>(bv->buffer->data) + bv->offset;
    px.reset(stbi_load_from_memory(bytes, static_cast<int>(bv->size), &w, &h,
                                   &c, 4));
  } else if (img.uri && std::strncmp(img.uri, "data:", 5) == 0) {
    const char *marker = std::strstr(img.uri, ";base64,");
    if (!marker) {
      return std::unexpected("image data URI is not base64");
    }
    const char *b64 = marker + 8;
    const std::size_t len = std::strlen(b64);
    const std::size_t pad =
        (len >= 1 && b64[len - 1] == '=') + (len >= 2 && b64[len - 2] == '=');
    const std::size_t size = len / 4 * 3 - pad;

    cgltf_options opts{};
    void *decoded = nullptr;
    if (cgltf_load_buffer_base64(&opts, size, b64, &decoded) !=
        cgltf_result_success) {
      return std::unexpected("failed to decode base64 image");
    }
    px.reset(stbi_load_from_memory(static_cast<const stbi_uc *>(decoded),
                                   static_cast<int>(size), &w, &h, &c, 4));
    std::free(decoded); // cgltf's default allocator is malloc
  } else if (img.uri) {
    std::string uri = img.uri;
    cgltf_decode_uri(uri.data()); // undo %20 etc. in place
    uri.resize(std::strlen(uri.c_str()));
    const auto file = base_dir / uri;
    px.reset(stbi_load(file.string().c_str(), &w, &h, &c, 4));
    if (!px) {
      return std::unexpected(
          std::format("{}: {}", file.string(), stbi_failure_reason()));
    }
  } else {
    return std::unexpected("image has neither uri nor buffer view");
  }

  if (!px) {
    return std::unexpected(std::format("stb: {}", stbi_failure_reason()));
  }
  return make_bitmap(w, h, px.get());
}

// ---------------------------------------------------------------------------
// Geometry helpers
// ---------------------------------------------------------------------------
auto find_attribute(const cgltf_primitive &prim, cgltf_attribute_type type,
                    int set = 0) -> const cgltf_accessor * {
  for (cgltf_size i = 0; i < prim.attributes_count; ++i) {
    const auto &a = prim.attributes[i];
    if (a.type == type && a.index == set) {
      return a.data;
    }
  }
  return nullptr;
}

// Handles stride, normalized ints and sparse accessors.
auto unpack_floats(const cgltf_accessor *acc) -> std::vector<float> {
  std::vector<float> out(acc->count * cgltf_num_components(acc->type));
  cgltf_accessor_unpack_floats(acc, out.data(), out.size());
  return out;
}

// Triangle list indices for triangles / strips / fans, indexed or not.
// Empty result = not a triangle primitive.
auto triangle_list_indices(const cgltf_primitive &prim, cgltf_size vertex_count)
    -> std::vector<Index> {
  std::vector<Index> src;
  if (prim.indices) {
    src.resize(prim.indices->count);
    for (cgltf_size i = 0; i < src.size(); ++i) {
      src[i] = static_cast<Index>(cgltf_accessor_read_index(prim.indices, i));
    }
  } else {
    src.resize(vertex_count);
    for (cgltf_size i = 0; i < vertex_count; ++i) {
      src[i] = static_cast<Index>(i);
    }
  }

  std::vector<Index> out;
  switch (prim.type) {
  case cgltf_primitive_type_triangles:
    return src;
  case cgltf_primitive_type_triangle_strip:
    for (std::size_t i = 0; i + 2 < src.size(); ++i) {
      if (i & 1)
        out.insert(out.end(), {src[i + 1], src[i], src[i + 2]});
      else
        out.insert(out.end(), {src[i], src[i + 1], src[i + 2]});
    }
    break;
  case cgltf_primitive_type_triangle_fan:
    for (std::size_t i = 1; i + 1 < src.size(); ++i) {
      out.insert(out.end(), {src[i], src[i + 1], src[0]});
    }
    break;
  default:
    break;
  }
  return out;
}

// glTF spec: if NORMAL is missing, flat normals must be generated.
// Flat shading needs one normal per face, so the mesh is un-indexed:
// every triangle gets its own three vertices (bone data is duplicated along).
auto generate_flat_normals(GltfMesh &mesh) -> void {
  const bool has_bones = !mesh.bone_vertices.empty();
  std::vector<Vertex> vertices;
  std::vector<BoneVertex> bones;
  vertices.reserve(mesh.indices.size());
  if (has_bones)
    bones.reserve(mesh.indices.size());

  for (std::size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
    Vertex tri[3] = {mesh.vertices[mesh.indices[i]],
                     mesh.vertices[mesh.indices[i + 1]],
                     mesh.vertices[mesh.indices[i + 2]]};

    const glm::vec3 a = glm::make_vec3(tri[0].position);
    const glm::vec3 b = glm::make_vec3(tri[1].position);
    const glm::vec3 c = glm::make_vec3(tri[2].position);
    glm::vec3 n = glm::cross(b - a, c - a); // CCW front faces
    const float len = glm::length(n);
    n = len > 0.0f ? n / len
                   : glm::vec3{0.0f, 1.0f, 0.0f}; // degenerate triangle

    for (std::size_t k = 0; k < 3; ++k) {
      std::memcpy(tri[k].normal, &n[0], sizeof tri[k].normal);
      vertices.push_back(tri[k]);
      if (has_bones)
        bones.push_back(mesh.bone_vertices[mesh.indices[i + k]]);
    }
  }

  mesh.indices.resize(vertices.size());
  for (std::size_t i = 0; i < mesh.indices.size(); ++i) {
    mesh.indices[i] = static_cast<Index>(i);
  }
  mesh.vertices = std::move(vertices);
  if (has_bones)
    mesh.bone_vertices = std::move(bones);
}

// Returns nullopt for primitives we skip (points/lines).
auto convert_primitive(const cgltf_primitive &prim, const cgltf_data &data)
    -> std::expected<std::optional<GltfMesh>, std::string> {
  const auto *pos = find_attribute(prim, cgltf_attribute_type_position);
  if (!pos || pos->type != cgltf_type_vec3) {
    return std::unexpected("primitive without vec3 POSITION");
  }
  const auto *nrm = find_attribute(prim, cgltf_attribute_type_normal);
  const auto *uv = find_attribute(prim, cgltf_attribute_type_texcoord, 0);
  if (nrm && nrm->type != cgltf_type_vec3)
    nrm = nullptr;
  if (uv && uv->type != cgltf_type_vec2)
    uv = nullptr;

  auto indices = triangle_list_indices(prim, pos->count);
  if (indices.empty()) {
    return std::optional<GltfMesh>{};
  }

  const auto p = unpack_floats(pos);
  const auto n = nrm ? unpack_floats(nrm) : std::vector<float>{};
  const auto t = uv ? unpack_floats(uv) : std::vector<float>{};

  GltfMesh mesh;
  mesh.vertices.resize(pos->count); // value-initialized: missing normal/uv = 0
  for (cgltf_size i = 0; i < pos->count; ++i) {
    auto &v = mesh.vertices[i];
    std::memcpy(v.position, &p[i * 3], sizeof v.position);
    if (nrm)
      std::memcpy(v.normal, &n[i * 3], sizeof v.normal);
    if (uv)
      std::memcpy(v.texture_coordinate, &t[i * 2], sizeof v.texture_coordinate);
  }
  mesh.indices = std::move(indices);

  const auto *joints = find_attribute(prim, cgltf_attribute_type_joints);
  const auto *weights = find_attribute(prim, cgltf_attribute_type_weights);
  if (joints && weights && joints->type == cgltf_type_vec4 &&
      weights->type == cgltf_type_vec4 && joints->count == pos->count &&
      weights->count == pos->count) {
    const auto w = unpack_floats(weights);
    mesh.bone_vertices.resize(pos->count);
    for (cgltf_size i = 0; i < pos->count; ++i) {
      cgltf_uint j[4] = {};
      cgltf_accessor_read_uint(joints, i, j, 4);
      auto &b = mesh.bone_vertices[i];
      float sum = 0.0f;
      for (std::size_t k = 0; k < 4; ++k) {
        b.ids[k] = static_cast<int>(j[k]);
        b.weights[k] = w[i * 4 + k];
        sum += b.weights[k];
      }
      if (sum > 0.0f) {
        for (float &x : b.weights)
          x /= sum;
      } else {
        b.weights[0] = 1.0f;
      }
    }
  }

  // Must run last among the geometry steps (after bone weights, if read),
  // since it duplicates vertices.
  if (!nrm) {
    generate_flat_normals(mesh);
  }

  if (prim.material) {
    mesh.material = static_cast<std::size_t>(prim.material - data.materials);
  }

  return std::optional<GltfMesh>{std::move(mesh)};
}

// ---------------------------------------------------------------------------
// Node tree
// ---------------------------------------------------------------------------
struct CgltfContext {
  const cgltf_data *data;
  // glTF mesh index -> indices into GltfScene::meshes (one per kept primitive)
  std::vector<std::vector<std::size_t>> mesh_map;
  std::vector<std::string> node_names; // unique, indexed like data->nodes
};

auto node_index(const cgltf_data &data, const cgltf_node &node) -> std::size_t {
  return static_cast<std::size_t>(&node - data.nodes);
}

auto unique_node_names(const cgltf_data &data) -> std::vector<std::string> {
  std::vector<std::string> names;
  std::unordered_set<std::string> used;
  for (cgltf_size i = 0; i < data.nodes_count; ++i) {
    const auto &n = data.nodes[i];
    std::string name = (n.name && *n.name) ? n.name : std::format("node_{}", i);
    if (!used.insert(name).second) {
      name = std::format("{}#{}", name, i);
      used.insert(name);
    }
    names.push_back(std::move(name));
  }
  return names;
}

// ---------------------------------------------------------------------------
// Extras -> GltfMetaData
//
// Blender writes object custom properties into node.extras as a JSON object
// (glTF export: Include -> Data -> Custom Properties).
//   "tag": "door"        -> string
//   "key_id": 3          -> int   (no '.', 'e' or 'E'; fits in int32)
//   "speed": 2.5         -> float
//   "solid": true        -> bool
//   "door": {"id": 3}    -> flattened to "door.id" -> int
//   arrays, null         -> skipped (GltfMetaData has no array type)
// ---------------------------------------------------------------------------
class ExtrasParser {
public:
  ExtrasParser(std::string_view json, GltfMetaData &out)
      : _s(json), _out(out) {}

  auto parse() -> std::expected<void, std::string> {
    if (!consume('{')) {
      return fail("extras is not a JSON object");
    }
    return object_body("");
  }

private:
  std::string_view _s;
  std::size_t _i = 0;
  GltfMetaData &_out;

  auto fail(std::string_view what) const -> std::unexpected<std::string> {
    return std::unexpected(std::format("extras json, offset {}: {}", _i, what));
  }

  void ws() {
    while (_i < _s.size() && (_s[_i] == ' ' || _s[_i] == '\n' ||
                              _s[_i] == '\r' || _s[_i] == '\t')) {
      ++_i;
    }
  }

  auto peek() const -> char { return _i < _s.size() ? _s[_i] : '\0'; }

  auto consume(char c) -> bool {
    ws();
    if (peek() == c) {
      ++_i;
      return true;
    }
    return false;
  }

  auto literal(std::string_view lit) -> bool {
    if (_s.substr(_i, lit.size()) == lit) {
      _i += lit.size();
      return true;
    }
    return false;
  }

  // Called after '{' has been consumed.
  auto object_body(const std::string &prefix)
      -> std::expected<void, std::string> {
    if (consume('}')) {
      return {};
    }
    while (true) {
      ws();
      auto key = string();
      if (!key) {
        return std::unexpected(std::move(key.error()));
      }
      if (!consume(':')) {
        return fail("expected ':'");
      }
      const auto full = prefix.empty() ? *key : prefix + "." + *key;
      if (auto r = value(full); !r) {
        return r;
      }
      if (consume(','))
        continue;
      if (consume('}'))
        return {};
      return fail("expected ',' or '}'");
    }
  }

  auto value(const std::string &key) -> std::expected<void, std::string> {
    ws();
    switch (peek()) {
    case '"': {
      auto s = string();
      if (!s) {
        return std::unexpected(std::move(s.error()));
      }
      _out.set_string(key, *s);
      return {};
    }
    case '{':
      ++_i;
      return object_body(key);
    case '[':
      return skip();
    case 't':
      if (literal("true")) {
        _out.set_bool(key, true);
        return {};
      }
      break;
    case 'f':
      if (literal("false")) {
        _out.set_bool(key, false);
        return {};
      }
      break;
    case 'n':
      if (literal("null"))
        return {};
      break;
    default:
      return number(key);
    }
    return fail("invalid literal");
  }

  auto number(const std::string &key) -> std::expected<void, std::string> {
    const auto start = _i;
    bool is_float = false;
    while (_i < _s.size()) {
      const char c = _s[_i];
      if (c == '.' || c == 'e' || c == 'E') {
        is_float = true;
      } else if (!(c == '-' || c == '+' || (c >= '0' && c <= '9'))) {
        break;
      }
      ++_i;
    }
    const auto text = _s.substr(start, _i - start);
    if (text.empty()) {
      return fail("unexpected character");
    }
    const char *b = text.data();
    const char *e = b + text.size();

    if (!is_float) {
      std::int64_t v{};
      const auto [p, ec] = std::from_chars(b, e, v);
      if (ec == std::errc{} && p == e &&
          v >= std::numeric_limits<std::int32_t>::min() &&
          v <= std::numeric_limits<std::int32_t>::max()) {
        _out.set_int(key, static_cast<std::int32_t>(v));
        return {};
      }
      // out of int32 range: fall through and store as float
    }

    float f{};
    const auto [p, ec] = std::from_chars(b, e, f);
    if (ec != std::errc{} || p != e) {
      return fail("invalid number");
    }
    _out.set_float(key, f);
    return {};
  }

  auto hex4() -> std::optional<std::uint32_t> {
    if (_i + 4 > _s.size()) {
      return std::nullopt;
    }
    std::uint32_t v{};
    const auto [p, ec] =
        std::from_chars(_s.data() + _i, _s.data() + _i + 4, v, 16);
    if (ec != std::errc{} || p != _s.data() + _i + 4) {
      return std::nullopt;
    }
    _i += 4;
    return v;
  }

  static void append_utf8(std::string &out, std::uint32_t cp) {
    if (cp < 0x80) {
      out += static_cast<char>(cp);
    } else if (cp < 0x800) {
      out += static_cast<char>(0xC0 | (cp >> 6));
      out += static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
      out += static_cast<char>(0xE0 | (cp >> 12));
      out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
      out += static_cast<char>(0x80 | (cp & 0x3F));
    } else {
      out += static_cast<char>(0xF0 | (cp >> 18));
      out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
      out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
      out += static_cast<char>(0x80 | (cp & 0x3F));
    }
  }

  auto string() -> std::expected<std::string, std::string> {
    if (peek() != '"') {
      return fail("expected string");
    }
    ++_i;
    std::string out;
    while (_i < _s.size()) {
      const char c = _s[_i++];
      if (c == '"') {
        return out;
      }
      if (c != '\\') {
        out += c;
        continue;
      }
      if (_i >= _s.size()) {
        break;
      }
      switch (const char esc = _s[_i++]; esc) {
      case '"':
        out += '"';
        break;
      case '\\':
        out += '\\';
        break;
      case '/':
        out += '/';
        break;
      case 'b':
        out += '\b';
        break;
      case 'f':
        out += '\f';
        break;
      case 'n':
        out += '\n';
        break;
      case 'r':
        out += '\r';
        break;
      case 't':
        out += '\t';
        break;
      case 'u': {
        auto hi = hex4();
        if (!hi)
          return fail("bad \\u escape");
        std::uint32_t cp = *hi;
        if (cp >= 0xD800 && cp <= 0xDBFF && literal("\\u")) { // surrogate pair
          auto lo = hex4();
          if (!lo)
            return fail("bad \\u escape");
          cp = 0x10000 + ((cp - 0xD800) << 10) + (*lo - 0xDC00);
        }
        append_utf8(out, cp);
        break;
      }
      default:
        return fail("bad escape");
      }
    }
    return fail("unterminated string");
  }

  // Skips any JSON value (used for arrays).
  auto skip() -> std::expected<void, std::string> {
    ws();
    const char c = peek();
    if (c == '"') {
      auto s = string();
      if (!s)
        return std::unexpected(std::move(s.error()));
      return {};
    }
    if (c == '{' || c == '[') {
      const char close = (c == '{') ? '}' : ']';
      ++_i;
      if (consume(close)) {
        return {};
      }
      while (true) {
        if (c == '{') {
          ws();
          auto k = string();
          if (!k)
            return std::unexpected(std::move(k.error()));
          if (!consume(':'))
            return fail("expected ':'");
        }
        if (auto r = skip(); !r)
          return r;
        if (consume(','))
          continue;
        if (consume(close))
          return {};
        return fail("expected ',' or closing bracket");
      }
    }
    // number / true / false / null
    while (_i < _s.size() && _s[_i] != ',' && _s[_i] != '}' && _s[_i] != ']' &&
           _s[_i] != ' ' && _s[_i] != '\t' && _s[_i] != '\r' &&
           _s[_i] != '\n') {
      ++_i;
    }
    return {};
  }
};

// cgltf >= 1.14 fills extras.data; older versions only give offsets into the
// JSON.
auto extras_json(const cgltf_data &data, const cgltf_extras &extras)
    -> std::string_view {
  if (extras.data) {
    return extras.data;
  }
  if (extras.end_offset > extras.start_offset) {
    return {data.json + extras.start_offset,
            extras.end_offset - extras.start_offset};
  }
  return {};
}

auto convert_node(const cgltf_node &node, const CgltfContext &ctx)
    -> GltfModel {
  GltfModel model;
  const auto node_idx = static_cast<std::size_t>(&node - ctx.data->nodes);
  model.name = ctx.node_names[node_idx];

  float local[16];
  cgltf_node_transform_local(&node, local); // column-major, same as glm
  model.transform = glm::make_mat4(local);

  if (node.mesh) {
    const auto mesh_idx =
        static_cast<std::size_t>(node.mesh - ctx.data->meshes);
    model.meshes = ctx.mesh_map[mesh_idx];
  }
  // metadata from node.extras (JSON, e.g. Blender custom properties)
  if (const auto json = extras_json(*ctx.data, node.extras); !json.empty()) {
    ExtrasParser(json, model.metadata)
        .parse(); // returns std::expected<void, std::string>
  }

  model.children.reserve(node.children_count);
  for (cgltf_size i = 0; i < node.children_count; ++i) {
    model.children.push_back(convert_node(*node.children[i], ctx));
  }
  return model;
}

auto build_skeleton(const cgltf_node &node, const CgltfContext &ctx)
    -> animation::Skeleton {
  float local[16];
  cgltf_node_transform_local(&node, local);
  animation::Skeleton s(ctx.node_names[node_index(*ctx.data, node)],
                        glm::make_mat4(local));
  for (cgltf_size i = 0; i < node.children_count; ++i) {
    s.add_child(build_skeleton(*node.children[i], ctx));
  }
  return s;
}

// Inserted in joint order, so BoneInfo id == glTF joint index == BoneVertex id.
auto build_bone_infos(const cgltf_skin &skin, const CgltfContext &ctx)
    -> animation::BoneInfos {
  animation::BoneInfos infos;
  for (cgltf_size i = 0; i < skin.joints_count; ++i) {
    glm::mat4 ibm(1.0f);
    if (skin.inverse_bind_matrices) {
      cgltf_accessor_read_float(skin.inverse_bind_matrices, i,
                                glm::value_ptr(ibm), 16);
    }
    infos.insert_bone(ctx.node_names[node_index(*ctx.data, *skin.joints[i])],
                      ibm);
  }
  return infos;
}

// (time, value) pairs; handles all three glTF interpolation modes.
template <std::size_t N>
auto read_track(const cgltf_animation_sampler &s)
    -> std::vector<std::pair<double, std::array<float, N>>> {
  std::vector<std::pair<double, std::array<float, N>>> keys;
  const bool cubic = s.interpolation == cgltf_interpolation_type_cubic_spline;
  for (cgltf_size i = 0; i < s.input->count; ++i) {
    float t = 0.0f;
    cgltf_accessor_read_float(s.input, i, &t, 1);
    std::array<float, N> v{};
    // cubic spline stores [in-tangent, value, out-tangent] per key: take the
    // value
    cgltf_accessor_read_float(s.output, cubic ? i * 3 + 1 : i, v.data(), N);
    if (s.interpolation == cgltf_interpolation_type_step && !keys.empty()) {
      // hold the previous value until just before this key
      keys.push_back(
          {std::nextafter(static_cast<double>(t), 0.0), keys.back().second});
    }
    keys.push_back({t, v});
  }
  return keys;
}

// Bone::*_index() fails once time passes a track's last key, so every
// track must reach the end of the clip.
template <typename Key>
void extend_to(std::vector<Key> &keys, double duration) {
  if (keys.size() > 1 && keys.back().timeStamp < duration) {
    auto last = keys.back();
    last.timeStamp = duration;
    keys.push_back(last);
  }
}

auto load_animation(const cgltf_animation &anim, std::size_t index,
                    const CgltfContext &ctx, animation::Skeleton &skeleton,
                    animation::BoneInfos &bone_infos) -> GltfAnimation {
  double duration = 0.0;
  for (cgltf_size c = 0; c < anim.channels_count; ++c) {
    const auto *in = anim.channels[c].sampler->input;
    float last = 0.0f;
    cgltf_accessor_read_float(in, in->count - 1, &last, 1);
    duration = std::max(duration, static_cast<double>(last));
  }

  std::map<std::size_t, animation::Bone> bones; // one Bone per animated node
  for (cgltf_size c = 0; c < anim.channels_count; ++c) {
    const auto &ch = anim.channels[c];
    if (!ch.target_node || !ch.sampler)
      continue;
    const auto ni = node_index(*ctx.data, *ch.target_node);
    const auto &name = ctx.node_names[ni];
    auto &bone =
        bones.try_emplace(ni, name, bone_infos.find_bone_id(name).value_or(-1))
            .first->second;

    switch (ch.target_path) {
    case cgltf_animation_path_type_translation:
      for (const auto &[t, v] : read_track<3>(*ch.sampler))
        bone.m_Positions.push_back({glm::vec3(v[0], v[1], v[2]), t});
      break;
    case cgltf_animation_path_type_rotation:
      for (const auto &[t, v] :
           read_track<4>(*ch.sampler)) // glTF xyzw -> glm (w,x,y,z)
        bone.m_Rotations.push_back(
            {glm::normalize(glm::quat(v[3], v[0], v[1], v[2])), t});
      break;
    case cgltf_animation_path_type_scale:
      for (const auto &[t, v] : read_track<3>(*ch.sampler))
        bone.m_Scales.push_back({glm::vec3(v[0], v[1], v[2]), t});
      break;
    default: // morph target weights: not supported
      break;
    }
  }

  const double total = duration > 0.0 ? duration : 1.0; // avoid fmod(x, 0)
  auto clip = animation::Animation(animation::copy(skeleton),
                                   animation::TotalTicks(total),
                                   animation::TicksPerSecond(1.0));
  clip.set_bone_infos(bone_infos);

  for (auto &[ni, bone] : bones) {
    // Bone needs >= 1 key per path: use the node's rest pose for paths this
    // clip doesn't touch.
    const auto &node = ctx.data->nodes[ni];
    if (bone.m_Positions.empty())
      bone.m_Positions.push_back({node.has_translation
                                      ? glm::make_vec3(node.translation)
                                      : glm::vec3(0.0f),
                                  0.0});
    if (bone.m_Rotations.empty())
      bone.m_Rotations.push_back(
          {node.has_rotation ? glm::quat(node.rotation[3], node.rotation[0],
                                         node.rotation[1], node.rotation[2])
                             : glm::quat(1.0f, 0.0f, 0.0f, 0.0f),
           0.0});
    if (bone.m_Scales.empty())
      bone.m_Scales.push_back(
          {node.has_scale ? glm::make_vec3(node.scale) : glm::vec3(1.0f), 0.0});

    extend_to(bone.m_Positions, total);
    extend_to(bone.m_Rotations, total);
    extend_to(bone.m_Scales, total);
    clip.add_bone(std::move(bone));
  }

  std::string name = (anim.name && *anim.name)
                         ? anim.name
                         : std::format("animation_{}", index);
  return GltfAnimation{std::move(name), std::move(clip)};
}

auto Resources::load_gltf(std::filesystem::path path)
    -> std::expected<GltfScene, std::string> {
  if (!std::filesystem::is_regular_file(path)) {
    return std::unexpected(
        std::format("Invalid gltf path: '{}'", path.string()));
  }

  auto options = cgltf_options{};
  using cgltf_data_ptr = cgltf_data *;
  auto raw = cgltf_data_ptr{nullptr};

  const std::string path_str = path.string();
  const auto base_dir = path.parent_path();
  if (auto r = cgltf_parse_file(&options, path_str.c_str(), &raw);
      r != cgltf_result_success) {
    return std::unexpected(
        std::format("{}: parse failed: {}", path_str, result_string(r)));
  }

  CgltfPtr data(raw);
  if (auto r = cgltf_load_buffers(&options, data.get(), path_str.c_str());
      r != cgltf_result_success) {
    return std::unexpected(std::format("{}: loading buffers failed: {}",
                                       path_str, result_string(r)));
  }

  if (auto r = cgltf_validate(data.get()); r != cgltf_result_success) {
    return std::unexpected(
        std::format("{}: validation failed: {}", path_str, result_string(r)));
  }

  auto scene = GltfScene{};

  // --- Bitmaps: each glTF image decoded at most once, on first use --------
  std::vector<std::optional<std::size_t>> image_to_bitmap(data->images_count);

  auto get_bitmap =
      [&](const cgltf_image &img) -> std::expected<std::size_t, std::string> {
    const auto idx = static_cast<std::size_t>(&img - data->images);
    if (image_to_bitmap[idx]) {
      return *image_to_bitmap[idx];
    }
    auto bmp = load_image(img, base_dir);
    if (!bmp) {
      return std::unexpected(std::move(bmp.error()));
    }
    scene.bitmaps.push_back(std::move(*bmp));
    image_to_bitmap[idx] = scene.bitmaps.size() - 1;
    return scene.bitmaps.size() - 1;
  };

  // --- Materials: 1:1 with glTF materials, so primitive indices map directly
  scene.materials.reserve(data->materials_count);
  for (cgltf_size i = 0; i < data->materials_count; ++i) {
    const auto &m = data->materials[i];
    GltfMaterial mat;
    mat.name = (m.name && *m.name) ? m.name : std::format("material_{}", i);

    const cgltf_texture *tex = nullptr;
    if (m.has_pbr_metallic_roughness) {
      tex = m.pbr_metallic_roughness.base_color_texture.texture;
    } else if (m.has_pbr_specular_glossiness) {
      tex = m.pbr_specular_glossiness.diffuse_texture.texture;
    }
    if (tex && tex->image) {
      auto b = get_bitmap(*tex->image);
      if (!b) {
        return std::unexpected(std::format("{}: material '{}': {}", path_str,
                                           mat.name, b.error()));
      }
      mat.diffuse = *b;
    }
    scene.materials.push_back(std::move(mat));
  }

  // --- Meshes: every glTF primitive once; nodes reference them by index ---
  auto ctx = CgltfContext{data.get(), {}};
  ctx.node_names = unique_node_names(*data); // new

  ctx.mesh_map.resize(data->meshes_count);
  for (cgltf_size mi = 0; mi < data->meshes_count; ++mi) {
    const auto &gm = data->meshes[mi];
    for (cgltf_size pi = 0; pi < gm.primitives_count; ++pi) {
      auto mesh = convert_primitive(gm.primitives[pi], *data);
      if (!mesh) {
        return std::unexpected(std::format("{}: mesh {} primitive {}: {}",
                                           path_str, mi, pi, mesh.error()));
      }
      if (*mesh) {
        scene.meshes.push_back(std::move(**mesh));
        ctx.mesh_map[mi].push_back(scene.meshes.size() - 1);
      }
    }
  }

  // --- Node tree: roots of the default scene --------------------------------
  // 1) Collect the root nodes once, so both the model tree and the skeleton
  //    are built from exactly the same roots.
  const cgltf_scene *gs =
      data->scene ? data->scene
                  : (data->scenes_count ? &data->scenes[0] : nullptr);

  std::vector<const cgltf_node *> root_nodes;
  if (gs) {
    for (cgltf_size i = 0; i < gs->nodes_count; ++i) {
      root_nodes.push_back(gs->nodes[i]);
    }
  } else {
    for (cgltf_size i = 0; i < data->nodes_count; ++i) {
      if (!data->nodes[i].parent) {
        root_nodes.push_back(&data->nodes[i]);
      }
    }
  }

  // 2) Model tree: same result as before.
  scene.roots.reserve(root_nodes.size());
  for (const auto *node : root_nodes) {
    scene.roots.push_back(convert_node(*node, ctx));
  }

  // 3) Animations: only if the file has a skin and clips.
  if (data->skins_count > 0 && data->animations_count > 0) {
    const auto &skin = data->skins[0];
    if (skin.joints_count > max_animation_bones) {
      return std::unexpected(
          std::format("{}: skin has {} joints, shader supports 100", path_str,
                      skin.joints_count));
    }

    // name -> {id, inverse bind matrix}, ids in glTF joint order
    auto bone_infos = build_bone_infos(skin, ctx);

    // the whole node hierarchy, under one root
    animation::Skeleton skeleton("__gltf_root__", glm::mat4(1.0f));
    for (const auto *node : root_nodes) {
      skeleton.add_child(build_skeleton(*node, ctx));
    }

    // one GltfAnimation per clip, each with its own copy of skeleton + bone
    // infos
    for (cgltf_size a = 0; a < data->animations_count; ++a) {
      scene.animations.push_back(
          load_animation(data->animations[a], a, ctx, skeleton, bone_infos));
    }
  }

  return scene;
}

} // namespace bubble
