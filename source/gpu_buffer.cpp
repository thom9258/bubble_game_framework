#include "gpu_buffer.hpp"

#include "context.hpp"

namespace bubble {

GPUBuffer::GPUBuffer(vk::BufferUsageFlags usage, std::size_t size) {
  vk::MemoryPropertyFlags constexpr property_flags =
      vk::MemoryPropertyFlagBits::eDeviceLocal;

  const auto bufferInfo =
      vk::BufferCreateInfo{}.setSize(size).setUsage(usage).setSharingMode(
          vk::SharingMode::eExclusive);

  _buffer =
      ContextImpl::instance().device().createBufferUnique(bufferInfo, nullptr);

  vk::PhysicalDeviceMemoryProperties memProperties =
      ContextImpl::instance().physical_device().getMemoryProperties();
  vk::MemoryRequirements memRequirements =
      ContextImpl::instance().device().getBufferMemoryRequirements(
          _buffer.get());

  const auto memoryTypeIndex = ContextImpl::find_memory_type(
      memProperties, memRequirements.memoryTypeBits, property_flags);

  auto allocInfo = vk::MemoryAllocateInfo{}
                       .setAllocationSize(memRequirements.size)
                       .setMemoryTypeIndex(memoryTypeIndex);

  _memory =
      ContextImpl::instance().device().allocateMemoryUnique(allocInfo, nullptr);
  ContextImpl::instance().device().bindBufferMemory(_buffer.get(),
                                                    _memory.get(), 0);
  _size = size;
}

auto GPUBuffer::size() -> std::size_t { return _size; }

auto GPUBuffer::buffer() -> vk::Buffer { return _buffer.get(); }

} // namespace bubble
