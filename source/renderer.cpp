#include "renderer.hpp"
#include "context.hpp"
#include "resources.hpp"
#include "uniform_arena.hpp"

#include <filesystem>
#include <fstream>
#include <ranges>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_structs.hpp>

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

[[nodiscard]]
std::vector<std::uint32_t> read_spirv_source(std::filesystem::path path) {

  std::ifstream file(path, std::ios::ate | std::ios::binary);
  if (!file.is_open()) {
    return {};
  }

  size_t const bytecount = static_cast<size_t>(file.tellg());
  constexpr size_t scaling_factor = sizeof(uint32_t) / sizeof(char);
  size_t const read_times = bytecount / scaling_factor;
  std::vector<std::uint32_t> buffer;
  buffer.resize(read_times);
  file.seekg(0);
  file.read(reinterpret_cast<char *>(buffer.data()),
            sizeof(buffer[0]) * buffer.size());
  file.close();
  return buffer;
}

auto initialize_rendergraphs() -> bool {
  for (std::size_t i = 0; i < frames_in_flight; i++) {
    instance().rendergraphs.push_back(TaskGraph{});
  }

  return true;
}

auto initialize_uniform_arenas() -> bool {
  static constexpr const auto uniform_arena_size = 1024 * 10;
  for (std::size_t i = 0; i < frames_in_flight; i++) {
    instance().uniform_arenas.emplace_back(uniform_arena_size);
  }

  return true;
}

auto initialize_swapchain(InitializeInfo &info) -> bool {
  vk::SurfaceCapabilitiesKHR window_capabilities =
      context::physical_device().getSurfaceCapabilitiesKHR(
          context::instance().surface.get());

  if (window_capabilities.currentExtent.width ==
      std::numeric_limits<uint32_t>::max()) {
    LOG_ERROR("Provided window surface has a undefined size!");
    return false;
  }

  std::uint32_t surface_format_count{0};
  vk::Result result = context::physical_device().getSurfaceFormatsKHR(
      context::instance().surface.get(), &surface_format_count, nullptr);
  if (result != vk::Result::eSuccess) {
    LOG_ERROR("could not get surface format count!");
    return false;
  }

  std::vector<vk::SurfaceFormatKHR> available_surface_formats(
      surface_format_count);

  result = context::physical_device().getSurfaceFormatsKHR(
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
      context::device().createSwapchainKHRUnique(swapChainCreateInfo, nullptr);
  return true;
}

auto initialize_swapchain_images() -> bool {
  std::uint32_t image_count{0};
  vk::Result result = context::device().getSwapchainImagesKHR(
      instance().swapchain.get(), &image_count, nullptr);

  if (result != vk::Result::eSuccess) {
    LOG_ERROR("could not get swapchain image count!");
  }

  instance().swapchain_images.resize(image_count);
  result = context::device().getSwapchainImagesKHR(
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
    view = context::device().createImageViewUnique(imageViewCreateInfo);
  }

  return true;
}

auto initialize_synchronization() -> bool {
  instance().render_finished.resize(instance().swapchain_images.size());
  for (vk::UniqueSemaphore &semaphore : instance().render_finished) {
    semaphore = context::device().createSemaphoreUnique(
        vk::SemaphoreCreateInfo{}, nullptr);
  }

  instance().image_available.resize(frames_in_flight);
  for (vk::UniqueSemaphore &semaphore : instance().image_available) {
    semaphore = context::device().createSemaphoreUnique(
        vk::SemaphoreCreateInfo{}, nullptr);
  }

  // Note: fences starts off as signaled because we preemptively need to wait
  // for the first frame fence
  const auto fence_create_info =
      vk::FenceCreateInfo{}.setFlags(vk::FenceCreateFlagBits::eSignaled);
  instance().in_flight.resize(frames_in_flight);
  for (vk::UniqueFence &fence : instance().in_flight) {
    fence = context::device().createFenceUnique(fence_create_info, nullptr);
  }

  instance().presentbuffers.resize(instance().swapchain_images.size());

  auto init_commandbuffer_alloc_info =
      vk::CommandBufferAllocateInfo{}
          .setCommandPool(context::instance().commandpool.get())
          .setLevel(vk::CommandBufferLevel::ePrimary)
          .setCommandBufferCount(1);

  for (auto &commandbuffer : instance().presentbuffers) {
    commandbuffer = std::move(
        context::device()
            .allocateCommandBuffersUnique(init_commandbuffer_alloc_info)
            .front());
  }

  return true;
}

auto initialize_descriptorpool() -> bool {
  const auto uniform_buffer_count = 32 * frames_in_flight;
  const auto total_count = uniform_buffer_count;
  const auto sizes =
      std::array{vk::DescriptorPoolSize{}
                     .setDescriptorCount(uniform_buffer_count)
                     .setType(vk::DescriptorType::eUniformBufferDynamic)};

  const auto pool_info =
      vk::DescriptorPoolCreateInfo{}.setPoolSizes(sizes).setMaxSets(
          total_count);

  instance().pipe.pool =
      context::device().createDescriptorPoolUnique(pool_info);

  return true;
}

auto initialize_mainpass_attachments(InitializeInfo &info) -> bool {
  instance().mainpass.extent =
      vk::Extent2D{}.setWidth(info.width).setHeight(info.height);

  for (std::size_t i = 0; i < frames_in_flight; i++) {
    const auto color_usage = vk::ImageUsageFlagBits::eTransferDst |
                             vk::ImageUsageFlagBits::eTransferSrc |
                             vk::ImageUsageFlagBits::eSampled |
                             vk::ImageUsageFlagBits::eColorAttachment;

    instance().mainpass.color_attachments.emplace_back(
        instance().mainpass.extent, color_usage, vk::Format::eR8G8B8A8Srgb,
        vk::ImageAspectFlagBits::eColor);

    const auto depth_usage = vk::ImageUsageFlagBits::eTransferDst |
                             vk::ImageUsageFlagBits::eTransferSrc |
                             vk::ImageUsageFlagBits::eSampled |
                             vk::ImageUsageFlagBits::eDepthStencilAttachment;

    instance().mainpass.depth_attachments.emplace_back(
        instance().mainpass.extent, depth_usage, vk::Format::eD32Sfloat,
        vk::ImageAspectFlagBits::eDepth);
  }

  return true;
}

auto initialize_mainpass() -> bool {
  const auto color_attachment =
      vk::AttachmentDescription{}
          .setFlags(vk::AttachmentDescriptionFlags())
          .setFormat(vk::Format::eR8G8B8A8Srgb)
          .setSamples(vk::SampleCountFlagBits::e1)
          .setLoadOp(vk::AttachmentLoadOp::eClear)
          .setStoreOp(vk::AttachmentStoreOp::eStore)
          .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
          .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
          .setInitialLayout(vk::ImageLayout::eUndefined)
          .setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal);

  const auto depth_attachment =
      vk::AttachmentDescription{}
          .setFlags(vk::AttachmentDescriptionFlags())
          .setFormat(vk::Format::eD32Sfloat)
          .setSamples(vk::SampleCountFlagBits::e1)
          .setLoadOp(vk::AttachmentLoadOp::eClear)
          .setStoreOp(vk::AttachmentStoreOp::eDontCare)
          .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
          .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
          .setInitialLayout(vk::ImageLayout::eUndefined)
          .setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

  const auto color_reference =
      vk::AttachmentReference{}.setAttachment(0).setLayout(
          vk::ImageLayout::eColorAttachmentOptimal);

  const auto depth_reference =
      vk::AttachmentReference{}.setAttachment(1).setLayout(
          vk::ImageLayout::eDepthStencilAttachmentOptimal);

  const auto subpass =
      vk::SubpassDescription{}
          .setFlags(vk::SubpassDescriptionFlags())
          .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
          .setInputAttachments({})
          .setResolveAttachments({})
          .setColorAttachments(color_reference)
          .setPDepthStencilAttachment(&depth_reference);

  const auto color_depth_dependency =
      vk::SubpassDependency{}
          .setSrcSubpass(vk::SubpassExternal)
          .setDstSubpass(0)
          .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput |
                           vk::PipelineStageFlagBits::eEarlyFragmentTests)
          .setSrcAccessMask(vk::AccessFlags())
          .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput |
                           vk::PipelineStageFlagBits::eEarlyFragmentTests)
          .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite |
                            vk::AccessFlagBits::eDepthStencilAttachmentWrite);

  std::array<vk::AttachmentDescription, 2> attachments{color_attachment,
                                                       depth_attachment};

  const auto dependencies = std::array{color_depth_dependency};
  const auto renderPassCreateInfo = vk::RenderPassCreateInfo{}
                                        .setFlags(vk::RenderPassCreateFlags())
                                        .setAttachments(attachments)
                                        .setDependencies(dependencies)
                                        .setSubpasses(subpass);

  instance().mainpass.renderpass =
      context::device().createRenderPassUnique(renderPassCreateInfo);

  for (std::size_t i = 0; i < frames_in_flight; i++) {
    const auto attachments =
        std::array{instance().mainpass.color_attachments[i].view(),
                   instance().mainpass.depth_attachments[i].view()};

    auto framebuffer_info =
        vk::FramebufferCreateInfo{}
            .setAttachments(attachments)
            .setWidth(instance().mainpass.extent.width)
            .setHeight(instance().mainpass.extent.height)
            .setLayers(1)
            .setRenderPass(instance().mainpass.renderpass.get());

    instance().mainpass.framebuffers.push_back(
        context::device().createFramebufferUnique(framebuffer_info));
  }

  return true;
}

auto initialize_flatcolor_pipe() -> bool {
  const auto vertex_path = std::filesystem::path("flatcolor.vert.spv");
  auto vertex_source = read_spirv_source(vertex_path);
  if (vertex_source.empty()) {
    LOG_ERROR("Could not read shader '{}'", vertex_path.string());
    return false;
  }

  const auto fragment_path = std::filesystem::path("flatcolor.frag.spv");
  auto fragment_source = read_spirv_source(fragment_path);
  if (fragment_source.empty()) {
    LOG_ERROR("Could not read shader '{}'", fragment_path.string());
    return false;
  }

  const auto vertex_module_info = vk::ShaderModuleCreateInfo{}
                                      .setFlags(vk::ShaderModuleCreateFlags())
                                      .setCode(vertex_source);

  const auto fragment_module_info = vk::ShaderModuleCreateInfo{}
                                        .setFlags(vk::ShaderModuleCreateFlags())
                                        .setCode(fragment_source);

  auto vertex_module =
      context::device().createShaderModuleUnique(vertex_module_info);
  auto fragment_module =
      context::device().createShaderModuleUnique(fragment_module_info);

  const auto shaderstage_infos =
      std::array{vk::PipelineShaderStageCreateInfo{}
                     .setStage(vk::ShaderStageFlagBits::eVertex)
                     .setFlags(vk::PipelineShaderStageCreateFlags())
                     .setModule(vertex_module.get())
                     .setPName("main"),
                 vk::PipelineShaderStageCreateInfo{}
                     .setStage(vk::ShaderStageFlagBits::eFragment)
                     .setFlags(vk::PipelineShaderStageCreateFlags())
                     .setModule(fragment_module.get())
                     .setPName("main")};

  const auto dynamic_states =
      std::array{vk::DynamicState::eViewport, vk::DynamicState::eScissor};

  const auto dynamic_state_info =
      vk::PipelineDynamicStateCreateInfo{}.setDynamicStates(dynamic_states);

  const auto vertex_bindings =
      std::array{vk::VertexInputBindingDescription{}
                     .setBinding(0)
                     .setStride(sizeof(Vertex))
                     .setInputRate(vk::VertexInputRate::eVertex)};

  const auto vertex_attributes =
      std::array{vk::VertexInputAttributeDescription{}
                     .setBinding(0)
                     .setLocation(0)
                     .setFormat(vk::Format::eR32G32B32Sfloat)
                     .setOffset(offsetof(Vertex, position)),
                 vk::VertexInputAttributeDescription{}
                     .setBinding(0)
                     .setLocation(1)
                     .setFormat(vk::Format::eR32G32B32Sfloat)
                     .setOffset(offsetof(Vertex, normal)),
                 vk::VertexInputAttributeDescription{}
                     .setBinding(0)
                     .setLocation(2)
                     .setFormat(vk::Format::eR32G32Sfloat)
                     .setOffset(offsetof(Vertex, texture_coordinate))};

  const auto vertex_input_state_info =
      vk::PipelineVertexInputStateCreateInfo{}
          .setFlags(vk::PipelineVertexInputStateCreateFlags())
          .setVertexBindingDescriptions(vertex_bindings)
          .setVertexAttributeDescriptions(vertex_attributes);

  const auto input_assembly_state_info =
      vk::PipelineInputAssemblyStateCreateInfo{}
          .setFlags(vk::PipelineInputAssemblyStateCreateFlags())
          .setPrimitiveRestartEnable(vk::False)
          .setTopology(vk::PrimitiveTopology::eTriangleList);

  const auto initial_viewport =
      vk::Viewport{}
          .setX(0.0f)
          .setY(0.0f)
          .setWidth(static_cast<float>(instance().mainpass.extent.width))
          .setHeight(static_cast<float>(instance().mainpass.extent.height))
          .setMinDepth(0.0f)
          .setMaxDepth(1.0f);

  const auto initial_scissor =
      vk::Rect2D{}.setOffset(vk::Offset2D{}.setX(0.0f).setY(0.0f));

  const auto viewport_state_info =
      vk::PipelineViewportStateCreateInfo{}
          .setFlags(vk::PipelineViewportStateCreateFlags())
          .setViewports(initial_viewport)
          .setScissors(initial_scissor);

  const auto rasterization_state_info =
      vk::PipelineRasterizationStateCreateInfo{}
          .setFlags(vk::PipelineRasterizationStateCreateFlags())
          .setDepthClampEnable(false)
          .setRasterizerDiscardEnable(false)
          .setPolygonMode(vk::PolygonMode::eFill)
          .setCullMode(vk::CullModeFlagBits::eBack)
          .setFrontFace(vk::FrontFace::eCounterClockwise)
          .setDepthBiasEnable(false)
          .setDepthBiasConstantFactor(0.0f)
          .setDepthBiasClamp(0.0f)
          .setDepthBiasSlopeFactor(0.0f)
          .setLineWidth(1.0f);

  auto multisample_state_info =
      vk::PipelineMultisampleStateCreateInfo{}
          .setFlags(vk::PipelineMultisampleStateCreateFlags())
          .setSampleShadingEnable(false)
          .setRasterizationSamples(vk::SampleCountFlagBits::e1);

  const auto color_component_flags =
      vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
      vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

  auto color_blend_attachment_state =
      vk::PipelineColorBlendAttachmentState{}
          .setBlendEnable(false)
          .setSrcColorBlendFactor(vk::BlendFactor::eOne)
          .setDstColorBlendFactor(vk::BlendFactor::eZero)
          .setColorBlendOp(vk::BlendOp::eAdd)
          .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
          .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
          .setAlphaBlendOp(vk::BlendOp::eAdd)
          .setColorWriteMask(color_component_flags);

  auto color_blend_state_info =
      vk::PipelineColorBlendStateCreateInfo{}
          .setFlags(vk::PipelineColorBlendStateCreateFlags())
          .setLogicOpEnable(false)
          .setLogicOp(vk::LogicOp::eNoOp)
          .setAttachments(color_blend_attachment_state)
          .setBlendConstants({1.0f, 1.0f, 1.0f, 1.0f});

  const auto modelinfo_bindings = std::array{
      vk::DescriptorSetLayoutBinding{}
          .setStageFlags(vk::ShaderStageFlagBits::eVertex)
          .setDescriptorType(vk::DescriptorType::eUniformBufferDynamic)
          .setBinding(0)
          .setDescriptorCount(1),
  };

  const auto modelinfo_layout_info =
      vk::DescriptorSetLayoutCreateInfo{}
          .setFlags(vk::DescriptorSetLayoutCreateFlags())
          .setBindings(modelinfo_bindings);

  instance().pipe.flatcolor.modelinfo_layout =
      context::device().createDescriptorSetLayoutUnique(modelinfo_layout_info,
                                                        nullptr);

  auto layout_info =
      vk::PipelineLayoutCreateInfo{}
          .setFlags(vk::PipelineLayoutCreateFlags())
          .setSetLayouts(instance().pipe.flatcolor.modelinfo_layout.get());

  instance().pipe.flatcolor.layout =
      context::device().createPipelineLayoutUnique(layout_info, nullptr);

  auto depth_stencil_state_info = vk::PipelineDepthStencilStateCreateInfo{}
                                      .setDepthTestEnable(true)
                                      .setDepthWriteEnable(true)
                                      .setDepthCompareOp(vk::CompareOp::eLess)
                                      .setDepthBoundsTestEnable(false)
                                      .setMinDepthBounds(0.0f)
                                      .setMaxDepthBounds(1.0f)
                                      .setStencilTestEnable(false);

  auto pipeline_info = vk::GraphicsPipelineCreateInfo{}
                           .setFlags(vk::PipelineCreateFlags())
                           .setStages(shaderstage_infos)
                           .setPVertexInputState(&vertex_input_state_info)
                           .setPInputAssemblyState(&input_assembly_state_info)
                           .setPTessellationState(nullptr)
                           .setPViewportState(&viewport_state_info)
                           .setPRasterizationState(&rasterization_state_info)
                           .setPMultisampleState(&multisample_state_info)
                           .setPDepthStencilState(&depth_stencil_state_info)
                           .setPColorBlendState(&color_blend_state_info)
                           .setPDynamicState(&dynamic_state_info)
                           .setLayout(instance().pipe.flatcolor.layout.get())
                           .setRenderPass(instance().mainpass.renderpass.get());

  auto result =
      context::device().createGraphicsPipelineUnique(nullptr, pipeline_info);

  if (result.result != vk::Result::eSuccess) {
    LOG_ERROR("Could not create graphics pipeline");
    return false;
  }

  instance().pipe.flatcolor.pipeline = std::move(result.value);
  return true;
}

auto initialize_flatcolor_sets() -> bool {
  instance().pipe.flatcolor.modelinfo_sets.resize(2);
  std::vector<vk::WriteDescriptorSet> writes;
  for (std::size_t i = 0; i < frames_in_flight; i++) {
    const auto alloc_info =
        vk::DescriptorSetAllocateInfo{}
            .setDescriptorSetCount(1)
            .setDescriptorPool(instance().pipe.pool.get())
            .setSetLayouts(instance().pipe.flatcolor.modelinfo_layout.get())
            .setDescriptorPool(instance().pipe.pool.get());

    instance().pipe.flatcolor.modelinfo_sets[i] =
        context::device().allocateDescriptorSets(alloc_info).front();

    const auto buffer_info =
        vk::DescriptorBufferInfo{}
            .setBuffer(instance().uniform_arenas[i].buffer())
            .setRange(sizeof(MVP))
            .setOffset(0);

    const auto write =
        vk::WriteDescriptorSet{}
            .setDescriptorType(vk::DescriptorType::eUniformBufferDynamic)
            .setDstSet(instance().pipe.flatcolor.modelinfo_sets[i])
            .setDstBinding(0)
            .setDescriptorCount(1)
            .setBufferInfo(buffer_info);

    writes.push_back(write);
  }

  context::device().updateDescriptorSets(writes.size(), writes.data(), 0,
                                         nullptr);
  return true;
}

auto initialize_renderbatches() -> bool {
  instance().renderbatches.resize(frames_in_flight);
  return true;
}

[[nodiscard]]
auto initialize(InitializeInfo &info) -> bool {
  singleton::ginstance = std::make_unique<Instance>();

  if (!initialize_rendergraphs()) {
    return false;
  }

  if (!initialize_uniform_arenas()) {
    return false;
  }

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

  if (!initialize_descriptorpool()) {
    return false;
  }

  if (!initialize_mainpass_attachments(info)) {
    return false;
  }

  if (!initialize_mainpass()) {
    return false;
  }

  if (!initialize_flatcolor_pipe()) {
    return false;
  }

  if (!initialize_flatcolor_sets()) {
    return false;
  }

  if (!initialize_renderbatches()) {
    return false;
  }

  return true;
}

auto destroy() -> void { singleton::ginstance = nullptr; }

auto begin_frame() -> void {
  const auto duration = instance().deltatime_timer.stop();

  if (duration.has_value()) {
    instance().deltatime = duration.value().count();
  }

  instance().deltatime_timer.start();
  instance().renderbatches[instance().flightframe].drawcommands.clear();
}

auto wait_for_next_frame() {
  const auto fences =
      std::array{instance().in_flight[instance().flightframe].get()};
  const auto max_wait = std::numeric_limits<unsigned int>::max();
  vk::Result wait_result =
      context::device().waitForFences(fences, true, max_wait);
  if (wait_result != vk::Result::eSuccess) {
    LOG_ERROR("Could not wait for swapchain");
  }

  context::device().resetFences(fences);
  const auto max_acquire_wait = std::numeric_limits<uint64_t>::max();
  auto result = context::device().acquireNextImageKHR(
      instance().swapchain.get(), max_acquire_wait,
      instance().image_available[instance().flightframe].get(), nullptr);

  if (result.result != vk::Result::eSuccess) {
    LOG_ERROR("Could not acquire next image from swapchain");
  }

  instance().image_index = result.value;
}

auto view_matrix() -> glm::mat4 {
  const auto camera = instance().camera;
  const auto target = camera.position() + camera.forward();
  return glm::lookAt(camera.position(), target, camera.up());
}

auto projection_matrix() -> glm::mat4 {
  const auto camera = instance().camera;
  const auto extent = instance().mainpass.extent;
  float aspect =
      static_cast<float>(extent.width) / static_cast<float>(extent.height);
  glm::mat4 proj =
      glm::perspective(camera.fov_y(), aspect, camera.near(), camera.far());
  proj[1][1] *= -1.0f;
  return proj;
}

auto end_frame() -> void {
  wait_for_next_frame();

  instance().uniform_arenas[instance().flightframe].reset();

  const auto draw_task = TaskID{0};
  TaskGraph &rendergraph = instance().rendergraphs[instance().flightframe];
  rendergraph = TaskGraph{};
  rendergraph.set_end(draw_task);
  rendergraph.add_task(draw_task, [&](vk::CommandBuffer commandbuffer) {
    const auto render_area =
        vk::Rect2D{}
            .setOffset(vk::Offset2D{}.setX(0.0f).setY(0.0f))
            .setExtent(instance().mainpass.extent);

    auto clearcolor = instance().clearcolor;

    const auto clearvalues = std::array<vk::ClearValue, 2>{
        vk::ClearColorValue{static_cast<float>(clearcolor.r) / 255.0f,
                            static_cast<float>(clearcolor.g) / 255.0f,
                            static_cast<float>(clearcolor.b) / 255.0f,
                            static_cast<float>(clearcolor.a) / 255.0f},
        vk::ClearDepthStencilValue{1.0f, 0}};

    const auto renderpass_begin_info =
        vk::RenderPassBeginInfo{}
            .setRenderPass(instance().mainpass.renderpass.get())
            .setFramebuffer(
                instance().mainpass.framebuffers[instance().flightframe].get())
            .setRenderArea(render_area)
            .setClearValues(clearvalues);

    commandbuffer.beginRenderPass(renderpass_begin_info,
                                  vk::SubpassContents::eInline);
    const auto extent = instance().mainpass.extent;

    const auto viewport = vk::Viewport{}
                              .setX(0)
                              .setY(0)
                              .setWidth(static_cast<float>(extent.width))
                              .setHeight(static_cast<float>(extent.height))
                              .setMinDepth(0.0f)
                              .setMaxDepth(1.0f);

    const auto scissor =
        vk::Rect2D{}.setOffset({0, 0}).setExtent({extent.width, extent.height});

    commandbuffer.setViewport(0, viewport);
    commandbuffer.setScissor(0, scissor);
    commandbuffer.bindPipeline(vk::PipelineBindPoint::eGraphics,
                               instance().pipe.flatcolor.pipeline.get());

    auto &drawcommands =
        instance().renderbatches[instance().flightframe].drawcommands;
    for (auto &command : drawcommands) {
      auto *mesh = resources::get(command.mesh);
      if (!mesh) {

        LOG_ERROR("Got invalid mesh in draw commands");
        continue;
      }

      {
        std::vector<vk::DeviceSize> offsets = {0};
        std::vector<vk::Buffer> buffers = {mesh->vertices.buffer()};
        commandbuffer.bindVertexBuffers(0, 1, buffers.data(), offsets.data());
      }

      {
        vk::DeviceSize offset = 0;
        vk::Buffer buffer = mesh->indices.buffer();
        commandbuffer.bindIndexBuffer(buffer, offset, vk::IndexType::eUint32);
      }

      {
        auto mvp = MVP{};
        mvp.model = command.location;
        mvp.view = view_matrix();
        mvp.color = glm::vec3{static_cast<float>(command.color.r) / 255,
                              static_cast<float>(command.color.g) / 255,
                              static_cast<float>(command.color.b) / 255};
        mvp.projection = projection_matrix();

        auto allocation =
            instance().uniform_arenas[instance().flightframe].allocate(
                sizeof(mvp));
        if (!allocation.has_value()) {
          LOG_ERROR("Ran out of uniform arena memory");
          continue;
        }

        std::memcpy(allocation->ptr, &mvp, sizeof(mvp));

        const auto layout = instance().pipe.flatcolor.layout.get();
        const auto first_set = std::uint32_t{0};
        const auto sets = std::array{
            instance().pipe.flatcolor.modelinfo_sets[instance().flightframe]};
        const auto dynamic_offsets =
            std::array{static_cast<std::uint32_t>(allocation->dynamic_offset)};
        commandbuffer.bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics, layout, first_set,
            static_cast<std::uint32_t>(sets.size()), sets.data(),
            static_cast<std::uint32_t>(dynamic_offsets.size()),
            dynamic_offsets.data());
      }

      commandbuffer.drawIndexed(mesh->indices_count, 1, 0, 0, 0);
    }

    commandbuffer.endRenderPass();
  });

  auto rendergraph_finished_signal = rendergraph.evaluate();

  vk::CommandBuffer commandbuffer =
      instance().presentbuffers[instance().flightframe].get();

  commandbuffer.reset();
  auto begin_info = vk::CommandBufferBeginInfo{};
  commandbuffer.begin(begin_info);

  {
    auto range = vk::ImageSubresourceRange{}
                     .setAspectMask(vk::ImageAspectFlagBits::eColor)
                     .setBaseMipLevel(0)
                     .setLevelCount(1)
                     .setBaseArrayLayer(0)
                     .setLayerCount(1);

    auto barrier =
        vk::ImageMemoryBarrier{}
            .setImage(instance().swapchain_images[instance().image_index])
            .setSubresourceRange(range)
            .setOldLayout(vk::ImageLayout::eUndefined)
            .setNewLayout(vk::ImageLayout::eTransferDstOptimal)
            .setSrcAccessMask(vk::AccessFlagBits::eTransferRead)
            .setDstAccessMask(vk::AccessFlags())
            .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);

    commandbuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                  vk::PipelineStageFlagBits::eTransfer,
                                  vk::DependencyFlags(), nullptr, nullptr,
                                  barrier);
  }

  Texture &texture_to_present =
      instance().mainpass.color_attachments[instance().flightframe];

  {
    auto range = vk::ImageSubresourceRange{}
                     .setAspectMask(vk::ImageAspectFlagBits::eColor)
                     .setBaseMipLevel(0)
                     .setLevelCount(1)
                     .setBaseArrayLayer(0)
                     .setLayerCount(1);

    auto barrier = vk::ImageMemoryBarrier{}
                       .setImage(texture_to_present.image())
                       .setSubresourceRange(range)
                       .setOldLayout(vk::ImageLayout::eColorAttachmentOptimal)
                       .setNewLayout(vk::ImageLayout::eTransferSrcOptimal)
                       .setSrcAccessMask(vk::AccessFlagBits::eTransferRead)
                       .setDstAccessMask(vk::AccessFlags())
                       .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
                       .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);

    commandbuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                  vk::PipelineStageFlagBits::eTransfer,
                                  vk::DependencyFlags(), nullptr, nullptr,
                                  barrier);
  }
  {
    auto src_subresource = vk::ImageSubresourceLayers{}
                               .setAspectMask(vk::ImageAspectFlagBits::eColor)
                               .setBaseArrayLayer(0)
                               .setLayerCount(1)
                               .setMipLevel(0);

    const auto src_offsets =
        std::array{vk::Offset3D{}.setX(0).setY(0).setZ(0),
                   vk::Offset3D{}
                       .setX(instance().mainpass.extent.width)
                       .setY(instance().mainpass.extent.height)
                       .setZ(1)};

    auto dst_subresource = vk::ImageSubresourceLayers{}
                               .setAspectMask(vk::ImageAspectFlagBits::eColor)
                               .setBaseArrayLayer(0)
                               .setLayerCount(1)
                               .setMipLevel(0);

    const auto dst_offsets =
        std::array{vk::Offset3D{}.setX(0).setY(0).setZ(0),
                   vk::Offset3D{}
                       .setX(instance().mainpass.extent.width)
                       .setY(instance().mainpass.extent.height)
                       .setZ(1)};

    auto image_blit = vk::ImageBlit{}
                          .setSrcOffsets(src_offsets)
                          .setSrcSubresource(src_subresource)
                          .setDstOffsets(dst_offsets)
                          .setDstSubresource(dst_subresource);

    commandbuffer.blitImage(
        texture_to_present.image(), vk::ImageLayout::eTransferSrcOptimal,
        instance().swapchain_images[instance().image_index],
        vk::ImageLayout::eTransferDstOptimal, image_blit, vk::Filter::eLinear);
  }
  {
    // Here we transfer the color attachment of the renderpass into
    // transfersrc so we can blit it to the swapchain
    auto to_present_range = vk::ImageSubresourceRange{}
                                .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                .setBaseMipLevel(0)
                                .setLevelCount(1)
                                .setBaseArrayLayer(0)
                                .setLayerCount(1);

    auto to_present_barrier =
        vk::ImageMemoryBarrier{}
            .setImage(instance().swapchain_images[instance().image_index])
            .setSubresourceRange(to_present_range)
            .setOldLayout(vk::ImageLayout::eTransferDstOptimal)
            .setNewLayout(vk::ImageLayout::ePresentSrcKHR)
            .setSrcAccessMask(vk::AccessFlagBits::eTransferRead)
            .setDstAccessMask(vk::AccessFlags())
            .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);

    commandbuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                  vk::PipelineStageFlagBits::eTransfer,
                                  vk::DependencyFlags(), nullptr, nullptr,
                                  to_present_barrier);
  }
  commandbuffer.end();

  // instance().render_finished[instance().image_index].get(),

  const auto wait_semaphores =
      std::array{instance().image_available[instance().flightframe].get(),
                 rendergraph_finished_signal};
  const auto wait_dst_stage_masks = std::array<vk::PipelineStageFlags, 2>{
      vk::PipelineStageFlagBits::eColorAttachmentOutput,
      vk::PipelineStageFlagBits::eTopOfPipe};

  const auto queue_signal_semaphores =
      std::array{instance().render_finished[instance().image_index].get()};
  auto submit_info = vk::SubmitInfo{}
                         .setWaitSemaphores(wait_semaphores)
                         .setWaitDstStageMask(wait_dst_stage_masks)
                         .setCommandBuffers({commandbuffer})
                         .setSignalSemaphores(queue_signal_semaphores);

  context::instance().queue.submit(
      submit_info, instance().in_flight[instance().flightframe].get());

  const auto swapchains = std::array{instance().swapchain.get()};
  const auto image_indices = std::array{instance().image_index};
  const auto present_wait_semaphores = queue_signal_semaphores;
  const auto present_info = vk::PresentInfoKHR{}
                                .setWaitSemaphores(present_wait_semaphores)
                                .setSwapchains(swapchains)
                                .setImageIndices(image_indices);

  vk::Result present_result =
      context::instance().queue.presentKHR(present_info);
  if (present_result != vk::Result::eSuccess) {
    LOG_ERROR("could not present frame");
  }

  instance().flightframe = (instance().flightframe + 1) % frames_in_flight;
}

auto set_clearcolor(Color color) -> void { instance().clearcolor = color; }

auto set_camera(Camera camera) -> void { instance().camera = camera; }

auto draw_flatcolor(glm::mat4 location, MeshRef mesh, Color color) -> void {
  instance().renderbatches[instance().flightframe].drawcommands.emplace_back(
      location, mesh, color);
}

} // namespace bubble::renderer
