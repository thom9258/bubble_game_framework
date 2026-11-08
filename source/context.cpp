#include "context.hpp"

#include <print>
#include <ranges>
#include <utility>

#if defined(_MSC_VER)
#include <direct.h>
#define getcwd _getcwd
#elif defined(__GNUC__)
#include <unistd.h>
#endif

namespace bubble {

static std::unique_ptr<ContextImpl> g_instance{nullptr};

auto ContextImpl::instance() -> ContextImpl & { return *g_instance; }

auto Context::destroy() -> void { g_instance.reset(); }

auto stringify(LogType type) {
  switch (type) {
  case LogType::info:
    return "info";
  case LogType::warn:
    return "warn";
  case LogType::error:
    return "error";
  }

  std::unreachable();
}

auto Context::program_path() -> std::optional<std::filesystem::path> {
  std::array<char, FILENAME_MAX> buffer;
  if (getcwd(buffer.data(), static_cast<int>(buffer.size())) != nullptr) {
    return std::filesystem::path(buffer.data());
  }

  return std::nullopt;
}

auto default_log_callback(LogType type, std::string_view msg) -> void {
  std::println("[{}] {}", stringify(type), msg);
}

auto GlobalContext::create() -> std::optional<GlobalContext> {
  if (SDL_Init(SDL_INIT_EVERYTHING) == 0) {
    LOG_ERROR("Could not init SDL");
    return std::nullopt;
  }

  if (SDL_Vulkan_LoadLibrary(nullptr) == 0) {
    LOG_ERROR("Could not load SDL Vulkan");
    return std::nullopt;
  }

  return GlobalContext{};
}

GlobalContext::~GlobalContext() {
  SDL_Vulkan_UnloadLibrary();
  SDL_Quit();
}

auto initialize_window(const InitializeConfig &config) -> bool {
  constexpr auto pos_x = int{0};
  constexpr auto pos_y = int{0};
  constexpr auto flags = 0 | SDL_WINDOW_VULKAN | SDL_WINDOW_ALLOW_HIGHDPI;

  ContextImpl::instance().window = SDLWindowPtr{SDL_CreateWindow(
      config.name.data(), pos_x, pos_y, config.width, config.height, flags)};

  if (ContextImpl::instance().window == nullptr) {
    return false;
  }

  return true;
}

auto window_instance_extensions() -> std::vector<const char *> {
  auto count = std::uint32_t{0};
  SDL_Vulkan_GetInstanceExtensions(ContextImpl::instance().window.get(), &count,
                                   nullptr);
  auto extensions = std::vector<const char *>(count);
  SDL_Vulkan_GetInstanceExtensions(ContextImpl::instance().window.get(), &count,
                                   extensions.data());
  return extensions;
}

auto initialize_vulkan_instance(const InitializeConfig &config) -> bool {
  auto instance_extensions = window_instance_extensions();
  instance_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

  LOG_INFO("Enabled instance extensions:");
  for (auto extension : instance_extensions) {
    LOG_INFO("  {}", extension);
  }

  auto validation_layers = std::vector<const char *>{
      "VK_LAYER_KHRONOS_validation",
  };

  const auto applicationInfo =
      vk::ApplicationInfo{}
          .setPApplicationName(config.name.c_str())
          .setPEngineName("engine")
          .setApplicationVersion(VK_MAKE_VERSION(1, 1, 0))
          .setEngineVersion(VK_MAKE_VERSION(1, 1, 0))
          .setApiVersion(VK_MAKE_VERSION(1, 1, 0));

  auto instanceCreateInfo =
      vk::InstanceCreateInfo{}
          .setPApplicationInfo(&applicationInfo)
          .setEnabledExtensionCount(instance_extensions.size())
          .setPpEnabledExtensionNames(instance_extensions.data());

  if (config.options.enable_graphics_validation) {
    instanceCreateInfo.setEnabledLayerCount(validation_layers.size());
    instanceCreateInfo.setPpEnabledLayerNames(validation_layers.data());
    LOG_INFO("Enabled validation layers:");
    for (auto layer : validation_layers) {
      LOG_INFO("  {}", layer);
    }
  }

  ContextImpl::instance().vkinstance =
      vk::createInstanceUnique(instanceCreateInfo);
  return true;
}

auto get_queue_family(vk::PhysicalDevice device, vk::SurfaceKHR surface)
    -> std::optional<std::uint32_t> {

  for (auto [i, family] :
       device.getQueueFamilyProperties() | std::views::enumerate) {

    bool const has_graphics =
        static_cast<bool>(family.queueFlags & vk::QueueFlagBits::eGraphics);
    bool const has_transfer =
        static_cast<bool>(family.queueFlags & vk::QueueFlagBits::eTransfer);
    bool const has_compute =
        static_cast<bool>(family.queueFlags & vk::QueueFlagBits::eCompute);
    bool const has_present = device.getSurfaceSupportKHR(i, surface);

    if (has_graphics && has_transfer && has_compute && has_present) {
      return i;
    }
  }

  return std::nullopt;
}

auto rate_physical_device(vk::PhysicalDevice physical_device,
                          vk::SurfaceKHR surface)
    -> std::optional<std::uint64_t> {
  std::optional<std::uint64_t> score{0};
  vk::PhysicalDeviceProperties properties = physical_device.getProperties();
  if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
    score.value() += 200;
  } else if (properties.deviceType == vk::PhysicalDeviceType::eIntegratedGpu) {
    score.value() += 10;
  }

  vk::PhysicalDeviceFeatures features = physical_device.getFeatures();
  if (!features.samplerAnisotropy) {
    score = std::nullopt;
  }
  if (!features.fillModeNonSolid) {
    score = std::nullopt;
  }

  vk::PhysicalDeviceMemoryProperties memory_properties =
      physical_device.getMemoryProperties();
  for (std::size_t i = 0; i < memory_properties.memoryTypeCount; i++) {
    if (memory_properties.memoryTypes[i].propertyFlags |
        vk::MemoryPropertyFlagBits::eDeviceLocal) {
      score.value() += 100;
    }
  }

  std::size_t constexpr gb = 1'000'000'000;

  for (std::size_t i = 0; i < memory_properties.memoryHeapCount; i++) {
    if (memory_properties.memoryHeaps[i].size > vk::DeviceSize(4 * gb)) {
      score.value() += 300;
    } else if (memory_properties.memoryHeaps[i].size > vk::DeviceSize(1 * gb)) {
      score.value() += 100;
    }
  }

  std::optional<std::uint32_t> family =
      get_queue_family(physical_device, surface);

  if (!family.has_value()) {
    return std::nullopt;
  }

  return score;
}

auto initialize_surface() -> bool {
  VkSurfaceKHR tmp;
  SDL_Vulkan_CreateSurface(ContextImpl::instance().window.get(),
                           ContextImpl::instance().vkinstance.get(), &tmp);
  ContextImpl::instance().surface = vk::UniqueSurfaceKHR(
      vk::SurfaceKHR(tmp), ContextImpl::instance().vkinstance.get());
  return true;
}

auto log_physical_device(vk::PhysicalDevice physical_device) -> void {
  const auto properties = physical_device.getProperties();
  const auto limits = properties.limits;
  LOG_INFO("{}:", properties.deviceName.data());
  LOG_INFO("  Type: {}", vk::to_string(properties.deviceType));
  LOG_INFO("  Max Sampler Anisotrophy: {}", limits.maxSamplerAnisotropy);
  LOG_INFO("  Max Dynamic Uniform Buffers: {}",
           limits.maxDescriptorSetUniformBuffersDynamic);
  LOG_INFO("  Max Memory Allocation Count: {}",
           limits.maxMemoryAllocationCount);

  static constexpr const auto gb = std::size_t{1'000'000'000};

  LOG_INFO("  Memory Heaps:");
  const auto memory_properties = physical_device.getMemoryProperties();
  for (std::size_t i = 0; i < memory_properties.memoryHeapCount; i++) {
    const auto size_gb =
        static_cast<double>(memory_properties.memoryHeaps[i].size) / gb;
    LOG_INFO("    Heap[{}]: {}gb", i, size_gb);
  }

  for (std::size_t i = 0; i < memory_properties.memoryTypeCount; i++) {
    LOG_INFO("    Type[{}]: {}", i,
             vk::to_string(memory_properties.memoryTypes[i].propertyFlags));
  }
}

auto initialize_physical_device() -> bool {
  std::uint32_t physical_device_count{0};
  vk::Result result =
      ContextImpl::instance().vkinstance.get().enumeratePhysicalDevices(
          &physical_device_count, nullptr);

  if (result != vk::Result::eSuccess) {
    LOG_ERROR("Could not enumerate physical devices");
    return false;
  }

  std::vector<vk::PhysicalDevice> physical_devices(physical_device_count);
  result = ContextImpl::instance().vkinstance.get().enumeratePhysicalDevices(
      &physical_device_count, physical_devices.data());

  if (result != vk::Result::eSuccess) {
    LOG_ERROR("Could not enumerate physical devices");
    return false;
  }

  using score_t = std::optional<std::uint64_t>;
  std::vector<score_t> scores(physical_devices.size());
  LOG_INFO("Physical Device Count: {}", physical_device_count);
  for (auto [i, physical_device] : physical_devices | std::views::enumerate) {
    log_physical_device(physical_device);
    scores[i] = rate_physical_device(physical_device,
                                     ContextImpl::instance().surface.get());
  }

  std::uint32_t best_score{0};
  LOG_INFO("Physical Device Scores:");
  for (auto [device, score] : std::views::zip(physical_devices, scores)) {
    if (!score.has_value())
      continue;

    if (score.value() > best_score) {
      best_score = score.value();
      ContextImpl::instance().m_physical_device = device;
    }

    vk::PhysicalDeviceProperties properties = device.getProperties();
    LOG_INFO("  {}) {}, score: {}", properties.deviceName.data(),
             vk::to_string(properties.deviceType), score.value());
  }
  if (best_score < 1) {
    LOG_ERROR("Could not get a suitable physical_device");
    return false;
  }

  const auto properties = ContextImpl::physical_device().getProperties();
  LOG_INFO("Chosen Physical Device: '{}'", properties.deviceName.data());
  return true;
}

auto initialize_queuefamily_index() -> bool {
  std::optional<std::uint32_t> family =
      get_queue_family(ContextImpl::instance().physical_device(),
                       ContextImpl::instance().surface.get());

  if (!family.has_value()) {
    LOG_ERROR("Could not get a queue family for physical_device");
    return false;
  }

  ContextImpl::instance().queuefamily_index = family.value();
  return true;
}

auto initialize_device() -> bool {
  std::array<float, 1> constexpr queue_priorities{1.0f};
  auto deviceQueueCreateInfo =
      vk::DeviceQueueCreateInfo{}
          .setFlags({})
          .setQueueFamilyIndex(ContextImpl::instance().queuefamily_index)
          .setQueuePriorities(queue_priorities)
          .setQueueCount(1);

  std::array<const char *, 1> constexpr device_extensions{
      VK_KHR_SWAPCHAIN_EXTENSION_NAME,
  };

  const auto features = vk::PhysicalDeviceFeatures{}
                            .setFillModeNonSolid(true)
                            .setSamplerAnisotropy(true);

  auto deviceCreateInfo =
      vk::DeviceCreateInfo{}
          .setQueueCreateInfoCount(1)
          .setQueueCreateInfos(deviceQueueCreateInfo)
          .setPEnabledFeatures(&features)
          .setPpEnabledExtensionNames(device_extensions.data())
          .setEnabledExtensionCount(device_extensions.size());

  ContextImpl::instance().m_device =
      ContextImpl::instance().physical_device().createDeviceUnique(
          deviceCreateInfo);
  return true;
}

auto initialize_queue() -> bool {
  constexpr int queue_index = 0;
  ContextImpl::instance().queue = ContextImpl::instance().device().getQueue(
      ContextImpl::instance().queuefamily_index, queue_index);
  return true;
}

auto initialize_commandpool() -> bool {
  auto commandPoolCreateInfo =
      vk::CommandPoolCreateInfo{}
          .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
          .setQueueFamilyIndex(ContextImpl::instance().queuefamily_index);

  ContextImpl::instance().commandpool =
      ContextImpl::instance().device().createCommandPoolUnique(
          commandPoolCreateInfo, nullptr);
  return true;
}

auto poll_events() -> void {
  auto events = std::vector<SDL_Event>{};
  auto current = SDL_Event{};
  while (SDL_PollEvent(&current)) {
    events.push_back(current);
  }

  ContextImpl::instance().events = events;
}

auto handle_events() -> void {
  for (SDL_Event event : ContextImpl::instance().events) {
    if (event.type == SDL_QUIT) {
      ContextImpl::instance().requested_close = true;
    }
  }
}

auto Context::begin_frame() -> void {
  poll_events();
  handle_events();
}

[[nodiscard]]
auto Context::requested_close() -> bool {
  return ContextImpl::instance().requested_close;
}

auto Context::initialize(const InitializeConfig &config) -> bool {
  if (g_instance != nullptr) {
    destroy();
  }

  g_instance.reset(new ContextImpl);

  ContextImpl::instance().globalcontext = GlobalContext::create();
  if (!ContextImpl::instance().globalcontext.has_value()) {
    return false;
  }

  if (!initialize_window(config)) {
    return false;
  }

  if (!initialize_vulkan_instance(config)) {
    return false;
  }

  if (!initialize_surface()) {
    return false;
  }

  if (!initialize_physical_device()) {
    return false;
  }

  if (!initialize_queuefamily_index()) {
    return false;
  }

  if (!initialize_device()) {
    return false;
  }

  if (!initialize_queue()) {
    return false;
  }

  if (!initialize_commandpool()) {
    return false;
  }

  return true;
}

auto ContextImpl::find_memory_type(
    vk::PhysicalDeviceMemoryProperties const &memoryProperties,
    uint32_t typeBits, vk::MemoryPropertyFlags requirementsMask)
    -> std::uint32_t {
  uint32_t typeIndex = uint32_t(~0);
  for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
    if ((typeBits & 1) && ((memoryProperties.memoryTypes[i].propertyFlags &
                            requirementsMask) == requirementsMask)) {
      typeIndex = i;
      break;
    }
    typeBits >>= 1;
  }

  if (typeIndex == uint32_t(~0)) {
    LOG_ERROR("No memory type could be found");
  }
  return typeIndex;
}

auto ContextImpl::device() -> vk::Device { return instance().m_device.get(); }

auto ContextImpl::physical_device() -> vk::PhysicalDevice {
  return instance().m_physical_device;
}

} // namespace bubble

namespace bubble {

auto get_sdl_events() -> std::vector<SDL_Event> {
  return ContextImpl::instance().events;
}

} // namespace bubble
