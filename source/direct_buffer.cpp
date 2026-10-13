#include "direct_buffer.hpp"

#include "context.hpp"

namespace bubble {

DirectBuffer::DirectBuffer(vk::BufferUsageFlags usage, std::size_t size) {
  vk::MemoryPropertyFlags constexpr property_flags =
      vk::MemoryPropertyFlagBits::eHostVisible |
      vk::MemoryPropertyFlagBits::eHostCoherent;

  const auto bufferInfo =
      vk::BufferCreateInfo{}.setSize(size).setUsage(usage).setSharingMode(
          vk::SharingMode::eExclusive);

  _buffer =
      context::instance().device.get().createBufferUnique(bufferInfo, nullptr);

  vk::PhysicalDeviceMemoryProperties memProperties =
      context::instance().physical_device.getMemoryProperties();
  vk::MemoryRequirements memRequirements =
      context::instance().device.get().getBufferMemoryRequirements(
          _buffer.get());

  const auto memoryTypeIndex = bubble::context::find_memory_type(
      memProperties, memRequirements.memoryTypeBits, property_flags);

  auto allocInfo = vk::MemoryAllocateInfo{}
                       .setAllocationSize(memRequirements.size)
                       .setMemoryTypeIndex(memoryTypeIndex);

  _memory =
      context::instance().device.get().allocateMemoryUnique(allocInfo, nullptr);
  context::instance().device.get().bindBufferMemory(_buffer.get(),
                                                    _memory.get(), 0);
  _size = size;
  _ptr = context::instance().device.get().mapMemory(
      _memory.get(), 0, _size, vk::MemoryMapFlags());
}

auto DirectBuffer::size() -> std::size_t { return _size; }

auto DirectBuffer::ptr() -> void * { return _ptr; }

auto DirectBuffer::buffer() -> vk::Buffer { return _buffer.get(); }

} // namespace bubble
