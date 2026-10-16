#include "uniform_arena.hpp"
#include "context.hpp"

namespace bubble {

inline constexpr const auto uniform_arena_usage_flags =
    vk::BufferUsageFlagBits::eUniformBuffer |
    vk::BufferUsageFlagBits::eTransferDst;

UniformArena::UniformArena(std::size_t size)
    : _buffer{uniform_arena_usage_flags, size}, _used{0},
      _alignment{context::physical_device()
                     .getProperties()
                     .limits.minUniformBufferOffsetAlignment} {}

auto UniformArena::size() -> std::size_t { return _buffer.size(); }

auto UniformArena::used() -> std::size_t { return _used; }

auto UniformArena::buffer() -> vk::Buffer { return _buffer.buffer(); }

auto forward_align(std::size_t alignment, std::uintptr_t ptr)
    -> std::uintptr_t {
  return (ptr + alignment - 1) & ~(alignment - 1);
}

auto UniformArena::allocate(std::size_t n) -> void * {
  const auto allocation_start = forward_align(_alignment, _used);
  const auto allocation_end = allocation_start + n;
  if (allocation_end >= size()) {
    return nullptr;
  }

  _used = allocation_end;
  const auto allocation_ptr =
      reinterpret_cast<std::uintptr_t>(_buffer.ptr()) + allocation_start;
  return reinterpret_cast<void *>(allocation_ptr);
}

auto UniformArena::reset() -> void { _used = 0; }

} // namespace bubble
