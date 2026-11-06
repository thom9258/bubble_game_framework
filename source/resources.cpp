#include "resources.hpp"

#include <bubble/bubble.hpp>

#include "context.hpp"
#include "direct_buffer.hpp"

#include <memory>

namespace bubble {

static std::unique_ptr<ResourcesImpl> m_instance{nullptr};

auto ResourcesImpl::instance() -> ResourcesImpl & { return *m_instance; }

[[nodiscard]]
auto Resources::initialize(const InitializeConfig&) -> bool {
  if (m_instance != nullptr) {
    destroy();
  }

  m_instance.reset(new ResourcesImpl);
  return true;
}

auto Resources::destroy() -> void { m_instance.reset(); }

auto ResourcesImpl::get(MeshRef ref) -> MeshSource * {
  return mesh_sources.get(Ref(ref.index(), ref.generation()));
}

auto ResourcesImpl::get(TextureRef ref) -> TextureSource * {
  return texture_sources.get(Ref(ref.index(), ref.generation()));
}

MeshRef::MeshRef(std::uint32_t index, std::uint32_t generation)
    : _index{index}, _generation{generation} {}

auto MeshRef::index() const -> std::uint32_t { return _index; }

auto MeshRef::generation() const -> std::uint32_t { return _generation; }

TextureRef::TextureRef(std::uint32_t index, std::uint32_t generation)
    : _index{index}, _generation{generation} {}

auto TextureRef::index() const -> std::uint32_t { return _index; }

auto TextureRef::generation() const -> std::uint32_t { return _generation; }

auto Resources::create_mesh(std::span<Vertex> vertices,
                            std::span<Index> indices)
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
      GPUBuffer(indices_buffer_usage, vertices_allocation_size);

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

} // namespace bubble
