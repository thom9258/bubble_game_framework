#pragma once

#include "direct_buffer.hpp"

namespace bubble
{

struct UniformArenaAllocation {
    void* ptr{nullptr};
    std::size_t dynamic_offset{0};
};

class UniformArena {
public:
  UniformArena(std::size_t size);
  auto size() -> std::size_t;
  auto used() -> std::size_t;
  auto buffer() -> vk::Buffer;
  auto allocate(std::size_t n) -> std::optional<UniformArenaAllocation>;
  auto reset() -> void;

private:
  DirectBuffer _buffer;
  std::size_t _used{0};
  std::size_t _alignment{0};
};


}
