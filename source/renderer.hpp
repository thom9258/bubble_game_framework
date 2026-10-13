#pragma once

#include "context.hpp"

namespace bubble::renderer {

inline constexpr const auto frames_in_flight = std::uint32_t{2};

struct Instance {
  vk::SurfaceFormatKHR swapchain_format;
  vk::UniqueSwapchainKHR swapchain;
  std::vector<vk::Image> swapchain_images;
  std::vector<vk::UniqueImageView> swapchain_imageviews;
  std::vector<vk::UniqueSemaphore> render_finished;
  std::vector<vk::UniqueSemaphore> image_available;
  std::vector<vk::UniqueFence> in_flight;
};

auto instance() -> Instance &;

[[nodiscard]]
auto initialize(InitializeInfo &info) -> bool;

auto destroy() -> void;

} // namespace bubble::renderer
