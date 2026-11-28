#include "resources.hpp"

#include <bubble/bubble.hpp>
#include <expected>
#include <filesystem>

#include "bubble/renderer.hpp"
#include "bubble/resources.hpp"
#include "context.hpp"
#include "direct_buffer.hpp"

#include <array>
#include <memory>
#include <span>

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

auto Resources::destroy() -> void {
  if (m_instance == nullptr) {
    return;
  }

  ContextImpl::device().waitIdle();

  m_instance.reset();
}

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
                  .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
                  .setDstAccessMask(vk::AccessFlagBits::eShaderRead);

          commandbuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                        vk::PipelineStageFlagBits::eFragmentShader,
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
  //TODO: This is a very bad fix for deleting stuff that is still in use.
  ContextImpl::device().waitIdle();
  ResourcesImpl::instance().texture_sources.remove(
      Ref(ref.index(), ref.generation()));
}

auto Resources::default_mesh() -> MeshRef {
  return ResourcesImpl::instance().default_mesh.value();
}

auto Resources::default_texture() -> TextureRef {
  return ResourcesImpl::instance().default_texture.value();
}

} // namespace bubble
