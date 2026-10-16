#pragma once

#include "direct_buffer.hpp"

namespace bubble
{

class UniformArena {
public:
  UniformArena(std::size_t size);
  auto size() -> std::size_t;
  auto used() -> std::size_t;
  auto buffer() -> vk::Buffer;
  auto allocate(std::size_t n) -> void*;
  auto reset() -> void;

private:
  DirectBuffer _buffer;
  std::size_t _used{0};
  std::size_t _alignment{0};
};


}
