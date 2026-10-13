#pragma once

#include "context.hpp"

namespace bubble {

class Texture {
public:
  Texture(vk::Extent2D extent, vk::ImageUsageFlags usage, vk::Format format,
          vk::ImageAspectFlags aspect);

  auto memory() -> vk::DeviceMemory;
  auto view() -> vk::ImageView;
  auto image() -> vk::Image;

private:
  vk::UniqueImage _image;
  vk::UniqueImageView _view;
  vk::UniqueDeviceMemory _memory;
};

} // namespace bubble
