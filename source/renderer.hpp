#pragma once

#include "context.hpp"
#include "task_graph.hpp"
#include "texture.hpp"
#include "uniform_arena.hpp"

namespace bubble::renderer {

inline constexpr const auto frames_in_flight = std::uint32_t{2};

struct DrawCommand {
  MeshRef mesh;
  Color color;
};

struct Instance {
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
      std::vector<vk::UniqueDescriptorSet> modelinfo_sets;
    } flatcolor;
  } pipe;

  Color clearcolor{255, 0, 0, 255};

  std::vector<TaskGraph> rendergraphs;
  std::vector<UniformArena> uniform_arenas;

  struct RenderBatch {
    std::vector<DrawCommand> drawcommands;
  };

  std::vector<RenderBatch> renderbatches;
};

auto instance() -> Instance &;

[[nodiscard]]
auto initialize(InitializeInfo &info) -> bool;

auto set_clearcolor(Color color) -> void;

auto begin_frame() -> void;

auto end_frame() -> void;

auto draw_flatcolor(MeshRef mesh, Color color) -> void;

auto destroy() -> void;

} // namespace bubble::renderer
