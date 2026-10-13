#pragma once

#include "sdl_vulkan.hpp"

namespace bubble {

class GPUBuffer {
public:
  GPUBuffer(vk::BufferUsageFlags usage, std::size_t size);
  auto size() -> std::size_t;
  auto buffer() -> vk::Buffer;

private:
  vk::UniqueBuffer _buffer;
  vk::UniqueDeviceMemory _memory;
  size_t _size{0};
};

} // namespace bubble
