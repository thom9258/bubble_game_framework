#pragma once

#include "sdl_vulkan.hpp"

namespace bubble {

class DirectBuffer {
public:
  DirectBuffer(vk::BufferUsageFlags usage, std::size_t size);
  auto size() -> std::size_t;
  auto ptr() -> void*;
  auto buffer() -> vk::Buffer;

private:
  vk::UniqueBuffer _buffer;
  vk::UniqueDeviceMemory _memory;
  size_t _size{0};
  void *_ptr{nullptr};
};

} // namespace bubble
