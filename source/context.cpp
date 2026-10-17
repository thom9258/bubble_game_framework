#include "context.hpp"

#include <print>
#include <ranges>
#include <utility>

namespace bubble::context {

namespace singleton {
static std::unique_ptr<Instance> ginstance{nullptr};
}

auto instance() -> Instance & { return *singleton::ginstance; }

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

auto initialize_window(InitializeInfo &info) -> bool {
  constexpr auto pos_x = int{0};
  constexpr auto pos_y = int{0};
  constexpr auto flags = 0 | SDL_WINDOW_VULKAN | SDL_WINDOW_ALLOW_HIGHDPI;

  instance().window = SDLWindowPtr{SDL_CreateWindow(
      info.name.data(), pos_x, pos_y, info.width, info.height, flags)};

  if (instance().window == nullptr) {
    return false;
  }

  return true;
}

auto window_instance_extensions() -> std::vector<const char *> {
  auto count = std::uint32_t{0};
  SDL_Vulkan_GetInstanceExtensions(instance().window.get(), &count, nullptr);
  auto extensions = std::vector<const char *>(count);
  SDL_Vulkan_GetInstanceExtensions(instance().window.get(), &count,
                                   extensions.data());
  return extensions;
}

auto initialize_vulkan_instance(InitializeInfo &info) -> bool {
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
          .setPApplicationName(info.name.c_str())
          .setPEngineName("engine")
          .setApplicationVersion(VK_MAKE_VERSION(1, 1, 0))
          .setEngineVersion(VK_MAKE_VERSION(1, 1, 0))
          .setApiVersion(VK_MAKE_VERSION(1, 1, 0));

  auto instanceCreateInfo =
      vk::InstanceCreateInfo{}
          .setPApplicationInfo(&applicationInfo)
          .setEnabledExtensionCount(instance_extensions.size())
          .setPpEnabledExtensionNames(instance_extensions.data());

  if (info.options.enable_graphics_validation) {
    instanceCreateInfo.setEnabledLayerCount(validation_layers.size());
    instanceCreateInfo.setPpEnabledLayerNames(validation_layers.data());
    LOG_INFO("Enabled validation layers:");
    for (auto layer : validation_layers) {
      LOG_INFO("  {}", layer);
    }
  }

  instance().instance = vk::createInstanceUnique(instanceCreateInfo);
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
  SDL_Vulkan_CreateSurface(instance().window.get(), instance().instance.get(),
                           &tmp);
  instance().surface =
      vk::UniqueSurfaceKHR(vk::SurfaceKHR(tmp), instance().instance.get());
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
  vk::Result result = instance().instance.get().enumeratePhysicalDevices(
      &physical_device_count, nullptr);

  if (result != vk::Result::eSuccess) {
    LOG_ERROR("Could not enumerate physical devices");
    return false;
  }

  std::vector<vk::PhysicalDevice> physical_devices(physical_device_count);
  result = instance().instance.get().enumeratePhysicalDevices(
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
    scores[i] = rate_physical_device(physical_device, instance().surface.get());
  }

  std::uint32_t best_score{0};
  LOG_INFO("Physical Device Scores:");
  for (auto [device, score] : std::views::zip(physical_devices, scores)) {
    if (!score.has_value())
      continue;

    if (score.value() > best_score) {
      best_score = score.value();
      instance().physical_device = device;
    }

    vk::PhysicalDeviceProperties properties = device.getProperties();
    LOG_INFO("  {}) {}, score: {}", properties.deviceName.data(),
             vk::to_string(properties.deviceType), score.value());
  }
  if (best_score < 1) {
    LOG_ERROR("Could not get a suitable physical_device");
    return false;
  }

  const auto properties = physical_device().getProperties();
  LOG_INFO("Chosen Physical Device: '{}'", properties.deviceName.data());
  return true;
}

auto initialize_queuefamily_index() -> bool {
  std::optional<std::uint32_t> family =
      get_queue_family(instance().physical_device, instance().surface.get());

  if (!family.has_value()) {
    LOG_ERROR("Could not get a queue family for physical_device");
    return false;
  }

  instance().queuefamily_index = family.value();
  return true;
}

auto initialize_device() -> bool {
  std::array<float, 1> constexpr queue_priorities{1.0f};
  auto deviceQueueCreateInfo =
      vk::DeviceQueueCreateInfo{}
          .setFlags({})
          .setQueueFamilyIndex(instance().queuefamily_index)
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

  instance().device =
      instance().physical_device.createDeviceUnique(deviceCreateInfo);
  return true;
}

auto initialize_queue() -> bool {
  constexpr int queue_index = 0;
  instance().queue = instance().device.get().getQueue(
      instance().queuefamily_index, queue_index);
  return true;
}

auto initialize_commandpool() -> bool {
  auto commandPoolCreateInfo =
      vk::CommandPoolCreateInfo{}
          .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
          .setQueueFamilyIndex(instance().queuefamily_index);

  instance().commandpool = instance().device.get().createCommandPoolUnique(
      commandPoolCreateInfo, nullptr);
  return true;
}

auto initialize(InitializeInfo &info) -> bool {
  singleton::ginstance = std::make_unique<Instance>();
  instance().globalcontext = GlobalContext::create();
  if (!instance().globalcontext.has_value()) {
    return false;
  }

  if (!initialize_window(info)) {
    return false;
  }

  if (!initialize_vulkan_instance(info)) {
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

auto destroy() -> void { singleton::ginstance = nullptr; }

auto poll_events() -> void {
  auto events = std::vector<SDL_Event>{};
  auto current = SDL_Event{};
  while (SDL_PollEvent(&current)) {
    events.push_back(current);
  }

  instance().events = events;
}

auto handle_events() -> void {
  for (SDL_Event event : instance().events) {
    if (event.type == SDL_QUIT) {
      instance().requested_close = true;
    }
  }
}

auto begin_frame() -> void {
  poll_events();
  handle_events();
}

auto requested_close() -> bool { return instance().requested_close; }

auto find_memory_type(
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

auto device() -> vk::Device { return instance().device.get(); }

auto physical_device() -> vk::PhysicalDevice {
  return instance().physical_device;
}

} // namespace bubble::context

namespace bubble {

auto get_sdl_events() -> std::vector<SDL_Event> {
  return context::instance().events;
}

} // namespace bubble
