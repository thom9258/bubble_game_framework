#pragma once

#include <bubble/context.hpp>

#include "sdl_vulkan.hpp"

#include <format>
#include <functional>
#include <memory>
#include <string_view>
#include <vector>

#define LOG_INFO(MSG, ...)                                                     \
  bubble::ContextImpl::instance().logcallback(LogType::info,                   \
                                              std::format(MSG, ##__VA_ARGS__))
#define LOG_WARN(MSG, ...)                                                     \
  bubble::ContextImpl::instance().logcallback(LogType::warn,                   \
                                              std::format(MSG, ##__VA_ARGS__))
#define LOG_ERROR(MSG, ...)                                                    \
  bubble::ContextImpl::instance().logcallback(LogType::error,                  \
                                              std::format(MSG, ##__VA_ARGS__))

namespace bubble {

auto default_log_callback(LogType, std::string_view msg) -> void;

struct SDLDeleter {
  void operator()(SDL_Window *p) const { SDL_DestroyWindow(p); }
  void operator()(SDL_Renderer *p) const { SDL_DestroyRenderer(p); }
  void operator()(SDL_Texture *p) const { SDL_DestroyTexture(p); }
};

using SDLWindowPtr = std::unique_ptr<SDL_Window, SDLDeleter>;

struct GlobalContext {
  static auto create() -> std::optional<GlobalContext>;
  ~GlobalContext();
};

class ContextImpl {
public:
  friend Context;

  static auto instance() -> ContextImpl &;

  static auto device() -> vk::Device;

  static auto physical_device() -> vk::PhysicalDevice;

  template <typename F>
    requires requires(F f, vk::CommandBuffer cmdb) { f(cmdb); }
  static constexpr auto immediate_evaluate(F &&f) -> vk::Result {

    auto init_commandbuffer_alloc_info =
        vk::CommandBufferAllocateInfo{}
            .setCommandPool(ContextImpl::instance().commandpool.get())
            .setLevel(vk::CommandBufferLevel::ePrimary)
            .setCommandBufferCount(1);

    vk::UniqueCommandBuffer commandbuffer = std::move(
        ContextImpl::instance()
            .device()
            .allocateCommandBuffersUnique(init_commandbuffer_alloc_info)
            .front());
    commandbuffer->begin(vk::CommandBufferBeginInfo{});
    std::invoke(std::forward<F>(f), commandbuffer.get());

    commandbuffer->end();
    auto commandbuffer_submit_info =
        vk::SubmitInfo{}.setCommandBuffers(commandbuffer.get());

    vk::UniqueFence fence = ContextImpl::instance().device().createFenceUnique(
        vk::FenceCreateInfo{});
    ContextImpl::instance().queue.submit(commandbuffer_submit_info,
                                         fence.get());
    const auto max_wait = std::numeric_limits<unsigned int>::max();
    return ContextImpl::instance().device().waitForFences(fence.get(), true,
                                                          max_wait);
  }

  static auto
  find_memory_type(vk::PhysicalDeviceMemoryProperties const &memoryProperties,
                   uint32_t typeBits, vk::MemoryPropertyFlags requirementsMask)
      -> std::uint32_t;

  LogCallback logcallback{default_log_callback};
  std::optional<GlobalContext> globalcontext;
  SDLWindowPtr window{nullptr};
  std::vector<SDL_Event> events{};
  bool requested_close{false};
  vk::UniqueInstance vkinstance;
  vk::UniqueSurfaceKHR surface;
  vk::PhysicalDevice m_physical_device;
  std::uint32_t queuefamily_index;
  vk::UniqueDevice m_device;
  vk::Queue queue;
  vk::UniqueCommandPool commandpool;

private:
  ContextImpl() = default;
};

auto begin_frame() -> void;

auto get_queue_family(vk::PhysicalDevice device, vk::SurfaceKHR surface)
    -> std::optional<std::uint32_t>;

} // namespace bubble
