#include "renderer.hpp"
#include "context.hpp"
#include <vulkan/vulkan_structs.hpp>

#include <ranges>

namespace bubble::renderer {

namespace singleton {
static std::unique_ptr<Instance> ginstance{nullptr};
}

auto instance() -> Instance & { return *singleton::ginstance; }

auto get_best_swapchain_surface_format(
    const std::span<vk::SurfaceFormatKHR> availables) -> vk::SurfaceFormatKHR {
  for (const auto &available : availables) {
    if (available.format == vk::Format::eB8G8R8A8Srgb &&
        available.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
      return available;
    }
  }

  return availables.front();
}

auto initialize_swapchain(InitializeInfo &info) -> bool {
  vk::SurfaceCapabilitiesKHR window_capabilities =
      context::instance().physical_device.getSurfaceCapabilitiesKHR(
          context::instance().surface.get());

  if (window_capabilities.currentExtent.width ==
      std::numeric_limits<uint32_t>::max()) {
    LOG_ERROR("Provided window surface has a undefined size!");
    return false;
  }

  std::uint32_t surface_format_count{0};
  vk::Result result = context::instance().physical_device.getSurfaceFormatsKHR(
      context::instance().surface.get(), &surface_format_count, nullptr);
  if (result != vk::Result::eSuccess) {
    LOG_ERROR("could not get surface format count!");
    return false;
  }

  std::vector<vk::SurfaceFormatKHR> available_surface_formats(
      surface_format_count);

  result = context::instance().physical_device.getSurfaceFormatsKHR(
      context::instance().surface.get(), &surface_format_count,
      available_surface_formats.data());
  if (result != vk::Result::eSuccess) {
    LOG_ERROR("could not get surface format count!");
    return false;
  }

  instance().swapchain_format =
      get_best_swapchain_surface_format(available_surface_formats);

  vk::PresentModeKHR const present_mode = (info.options.enable_vsync)
                                              ? vk::PresentModeKHR::eFifo
                                              : vk::PresentModeKHR::eImmediate;

  const auto supports_identity =
      static_cast<bool>(window_capabilities.supportedTransforms &
                        vk::SurfaceTransformFlagBitsKHR::eIdentity);

  const vk::SurfaceTransformFlagBitsKHR preTransform =
      (supports_identity) ? vk::SurfaceTransformFlagBitsKHR::eIdentity
                          : window_capabilities.currentTransform;

  // this spaghetti determines the best compositealpha flags....
  vk::CompositeAlphaFlagBitsKHR compositeAlpha =
      (window_capabilities.supportedCompositeAlpha &
       vk::CompositeAlphaFlagBitsKHR::ePreMultiplied)
          ? vk::CompositeAlphaFlagBitsKHR::ePreMultiplied
      : (window_capabilities.supportedCompositeAlpha &
         vk::CompositeAlphaFlagBitsKHR::ePostMultiplied)
          ? vk::CompositeAlphaFlagBitsKHR::ePostMultiplied
      : (window_capabilities.supportedCompositeAlpha &
         vk::CompositeAlphaFlagBitsKHR::eInherit)
          ? vk::CompositeAlphaFlagBitsKHR::eInherit
          : vk::CompositeAlphaFlagBitsKHR::eOpaque;

  std::uint32_t image_count{0};
  if (window_capabilities.maxImageCount > 0) {
    image_count =
        std::clamp(frames_in_flight, window_capabilities.minImageCount,
                   window_capabilities.maxImageCount);
  } else {
    image_count = window_capabilities.minImageCount;
  }

  auto swapChainCreateInfo =
      vk::SwapchainCreateInfoKHR{}
          .setFlags(vk::SwapchainCreateFlagsKHR())
          .setSurface(context::instance().surface.get())
          .setMinImageCount(image_count)
          .setImageFormat(instance().swapchain_format.format)
          .setImageColorSpace(instance().swapchain_format.colorSpace)
          .setImageExtent(
              vk::Extent2D{}.setWidth(info.width).setHeight(info.height))
          .setImageArrayLayers(1)
          .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment |
                         vk::ImageUsageFlagBits::eTransferSrc |
                         vk::ImageUsageFlagBits::eTransferDst |
                         vk::ImageUsageFlagBits::eSampled)
          .setClipped(true)
          .setPreTransform(preTransform)
          .setCompositeAlpha(compositeAlpha)
          .setPresentMode(present_mode)
          .setImageSharingMode(vk::SharingMode::eExclusive);

  instance().swapchain =
      context::instance().device.get().createSwapchainKHRUnique(
          swapChainCreateInfo, nullptr);
  return true;
}

auto initialize_swapchain_images() -> bool {
  std::uint32_t image_count{0};
  vk::Result result = context::instance().device.get().getSwapchainImagesKHR(
      instance().swapchain.get(), &image_count, nullptr);

  if (result != vk::Result::eSuccess) {
    LOG_ERROR("could not get swapchain image count!");
  }

  instance().swapchain_images.resize(image_count);
  result = context::instance().device.get().getSwapchainImagesKHR(
      instance().swapchain.get(), &image_count,
      instance().swapchain_images.data());

  if (result != vk::Result::eSuccess) {
    LOG_ERROR("could not get surface formats!");
  }

  return true;
}

auto initialize_swapchain_imageviews() -> bool {
  instance().swapchain_imageviews.resize(instance().swapchain_images.size());
  auto subresourceRange = vk::ImageSubresourceRange{}
                              .setAspectMask(vk::ImageAspectFlagBits::eColor)
                              .setBaseMipLevel(0)
                              .setLevelCount(1)
                              .setBaseArrayLayer(0)
                              .setLayerCount(1);

  auto componentMapping = vk::ComponentMapping{}
                              .setR(vk::ComponentSwizzle::eIdentity)
                              .setG(vk::ComponentSwizzle::eIdentity)
                              .setB(vk::ComponentSwizzle::eIdentity)
                              .setA(vk::ComponentSwizzle::eIdentity);

  auto imageViewCreateInfo = vk::ImageViewCreateInfo{}
                                 .setSubresourceRange(subresourceRange)
                                 .setViewType(vk::ImageViewType::e2D)
                                 .setFormat(instance().swapchain_format.format)
                                 .setComponents(componentMapping);

  for (auto [i, view] :
       instance().swapchain_imageviews | std::views::enumerate) {
    imageViewCreateInfo.setImage(instance().swapchain_images[i]);
    view = context::instance().device.get().createImageViewUnique(
        imageViewCreateInfo);
  }

  return true;
}

auto initialize_synchronization() -> bool {
  instance().render_finished.resize(instance().swapchain_images.size());
  for (vk::UniqueSemaphore &semaphore : instance().render_finished) {
    semaphore = context::instance().device.get().createSemaphoreUnique(
        vk::SemaphoreCreateInfo{}, nullptr);
  }

  instance().image_available.resize(frames_in_flight);
  for (vk::UniqueSemaphore &semaphore : instance().image_available) {
    semaphore = context::instance().device.get().createSemaphoreUnique(
        vk::SemaphoreCreateInfo{}, nullptr);
  }

  // Note: fences starts off as signaled because we preemptively need to wait
  // for the first frame fence
  const auto fence_create_info =
      vk::FenceCreateInfo{}.setFlags(vk::FenceCreateFlagBits::eSignaled);
  instance().in_flight.resize(frames_in_flight);
  for (vk::UniqueFence &fence : instance().in_flight) {
    fence = context::instance().device.get().createFenceUnique(
        fence_create_info, nullptr);
  }

  return true;
}

[[nodiscard]]
auto initialize(InitializeInfo &info) -> bool {
  LOG_INFO("Initializing renderer");
  singleton::ginstance = std::make_unique<Instance>();

  if (!initialize_swapchain(info)) {
    return false;
  }

  if (!initialize_swapchain_images()) {
    return false;
  }

  if (!initialize_swapchain_imageviews()) {
    return false;
  }

  if (!initialize_synchronization()) {
    return false;
  }
  return true;
}

auto destroy() -> void { singleton::ginstance = nullptr; }

} // namespace bubble::renderer
