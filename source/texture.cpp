#include "texture.hpp"
#include "context.hpp"

namespace bubble {

Texture::Texture(vk::Extent2D extent, vk::ImageUsageFlags usage,
                 vk::Format format, vk::ImageAspectFlags aspect) {
  const auto imageCreateInfo =
      vk::ImageCreateInfo{}
          .setImageType(vk::ImageType::e2D)
          .setFormat(format)
          .setExtent(vk::Extent3D(extent, 1))
          .setMipLevels(1)
          .setArrayLayers(1)
          .setTiling(vk::ImageTiling::eOptimal)
          .setUsage(usage)
          .setInitialLayout(vk::ImageLayout::eUndefined)
          .setSharingMode(vk::SharingMode::eExclusive)
          .setSamples(vk::SampleCountFlagBits::e1);

  _image = context::instance().device.get().createImageUnique(imageCreateInfo);

  vk::PhysicalDeviceMemoryProperties memProperties =
      context::instance().physical_device.getMemoryProperties();

  vk::MemoryRequirements memRequirements =
      context::instance().device.get().getImageMemoryRequirements(_image.get());

  const auto memoryTypeIndex =
      context::find_memory_type(memProperties, memRequirements.memoryTypeBits,
                                vk::MemoryPropertyFlagBits::eDeviceLocal);

  auto allocInfo = vk::MemoryAllocateInfo{}
                       .setAllocationSize(memRequirements.size)
                       .setMemoryTypeIndex(memoryTypeIndex);
  _memory =
      context::instance().device.get().allocateMemoryUnique(allocInfo, nullptr);

  context::instance().device.get().bindImageMemory(_image.get(), _memory.get(),
                                                   0);

  auto subresourceRange = vk::ImageSubresourceRange{}
                              .setAspectMask(aspect)
                              .setBaseMipLevel(0)
                              .setLevelCount(1)
                              .setBaseArrayLayer(0)
                              .setLayerCount(1);

  auto componentMapping = vk::ComponentMapping{}
                              .setR(vk::ComponentSwizzle::eIdentity)
                              .setG(vk::ComponentSwizzle::eIdentity)
                              .setB(vk::ComponentSwizzle::eIdentity)
                              .setA(vk::ComponentSwizzle::eIdentity);

  auto imageViewCreateInfo = vk::ImageViewCreateInfo{}
                                 .setSubresourceRange(subresourceRange)
                                 .setViewType(vk::ImageViewType::e2D)
                                 .setFormat(format)
                                 .setComponents(componentMapping)
                                 .setImage(_image.get());

  _view = context::instance().device.get().createImageViewUnique(
      imageViewCreateInfo);
}

auto Texture::memory() -> vk::DeviceMemory { return _memory.get(); }
auto Texture::view() -> vk::ImageView { return _view.get(); }
auto Texture::image() -> vk::Image { return _image.get(); }

} // namespace bubble
