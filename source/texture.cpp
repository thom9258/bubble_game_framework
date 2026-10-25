#include "texture.hpp"
#include "context.hpp"
#include <vulkan/vulkan_structs.hpp>

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

auto Texture::make_sampler() -> void {
  const auto features = context::physical_device().getFeatures();
  const auto properties = context::physical_device().getProperties();
  const auto max_anisotropy =
      features.samplerAnisotropy
          ? std::min(4.0f, properties.limits.maxSamplerAnisotropy)
          : 1.0f;

  const vk::Filter filter = vk::Filter::eNearest;
  const auto sampler_info =
      vk::SamplerCreateInfo{}
          .setMagFilter(filter)
          .setMinFilter(filter)
          .setAddressModeU(vk::SamplerAddressMode::eRepeat)
          .setAddressModeV(vk::SamplerAddressMode::eRepeat)
          .setAddressModeW(vk::SamplerAddressMode::eRepeat)
          .setAnisotropyEnable(features.samplerAnisotropy)
          .setMaxAnisotropy(max_anisotropy)
          .setBorderColor(vk::BorderColor::eIntOpaqueBlack)
          .setUnnormalizedCoordinates(false)
          .setCompareEnable(false)
          .setCompareOp(vk::CompareOp::eAlways)
          .setMipmapMode(vk::SamplerMipmapMode::eLinear)
          .setMipLodBias(0.0f)
          .setMinLod(0.0f)
          .setMaxLod(0.0f);

  _sampler = context::device().createSamplerUnique(sampler_info);
}

auto Texture::sampler() -> std::optional<vk::Sampler> {
  if (_sampler.has_value()) {
    return _sampler->get();
  }

  return std::nullopt;
}

} // namespace bubble
