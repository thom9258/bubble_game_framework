#pragma once

#include <bubble/internal/glm.hpp>
#include <bubble/timer.hpp>

#include "context.hpp"
#include "task_graph.hpp"
#include "texture.hpp"
#include "uniform_arena.hpp"

namespace bubble::renderer {

inline constexpr const auto frames_in_flight = std::uint32_t{2};

struct DrawCommand {
  glm::mat4 location;
  MeshRef mesh;
  Color color;
};

struct MVP {
  glm::mat4 model;
  glm::mat4 view;
  glm::mat4 projection;
  alignas(sizeof(glm::vec4)) glm::vec3 color;
};

struct Instance {
  Timer deltatime_timer;
  std::optional<float> deltatime;
  Color clearcolor{255, 0, 0, 255};
  Camera camera{};

  std::vector<TaskGraph> rendergraphs;
  std::vector<UniformArena> uniform_arenas;

  vk::SurfaceFormatKHR swapchain_format;
  vk::UniqueSwapchainKHR swapchain;
  std::vector<vk::Image> swapchain_images;
  std::vector<vk::UniqueImageView> swapchain_imageviews;
  std::vector<vk::UniqueSemaphore> render_finished;
  std::vector<vk::UniqueSemaphore> image_available;
  std::vector<vk::UniqueFence> in_flight;
  std::vector<vk::UniqueCommandBuffer> presentbuffers;
  std::uint32_t flightframe{0};
  std::uint32_t image_index{0};

  struct {
    vk::Extent2D extent;
    vk::UniqueRenderPass renderpass;
    std::vector<Texture> color_attachments;
    std::vector<Texture> depth_attachments;
    std::vector<vk::UniqueFramebuffer> framebuffers;
  } mainpass;

  struct {
    vk::UniqueDescriptorPool pool;

    struct {
      vk::UniquePipelineLayout layout;
      vk::UniquePipeline pipeline;
      vk::UniqueDescriptorSetLayout modelinfo_layout;
      std::vector<vk::DescriptorSet> modelinfo_sets;
    } flatcolor;
  } pipe;

  struct RenderBatch {
    std::vector<DrawCommand> drawcommands;
  };

  std::vector<RenderBatch> renderbatches;
};

auto instance() -> Instance &;

[[nodiscard]]
auto initialize(InitializeInfo &info) -> bool;

auto set_clearcolor(Color color) -> void;

auto set_camera(Camera camera) -> void;

auto begin_frame() -> void;

auto end_frame() -> void;

auto draw_flatcolor(glm::mat4 location, MeshRef mesh, Color color) -> void;

auto destroy() -> void;

} // namespace bubble::renderer
