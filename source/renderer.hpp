#pragma once

#include <bubble/internal/glm.hpp>
#include <bubble/timer.hpp>

#include "bubble/renderer.hpp"
#include "flat_sorted_set.hpp"
#include "task_graph.hpp"
#include "texture.hpp"
#include "uniform_arena.hpp"

namespace bubble {

inline constexpr const auto frames_in_flight = std::uint32_t{2};

struct DrawFlatColor {
  glm::mat4 location;
  MeshRef mesh;
  Color color;
};

struct DrawTexture {
  glm::mat4 location;
  MeshRef mesh;
  TextureRef diffuse;
};

struct FlatColorUniform {
  glm::mat4 model;
  glm::mat4 view;
  glm::mat4 projection;
  alignas(sizeof(glm::vec4)) glm::vec3 color;
};

struct TextureUniform {
  glm::mat4 model;
  glm::mat4 view;
  glm::mat4 projection;
  // DirectionalLight directional_light;
};

class RendererImpl {
public:
  friend Renderer;
  static auto instance() -> RendererImpl &;

  Timer deltatime_timer;
  std::optional<double> deltatime;
  Color clearcolor{255, 0, 0, 255};
  Camera camera{glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f)};

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

    struct {
      vk::UniquePipelineLayout layout;
      vk::UniquePipeline pipeline;
      vk::UniqueDescriptorSetLayout modelinfo_layout;
      vk::UniqueDescriptorSetLayout diffuse_layout;
      std::vector<vk::DescriptorSet> modelinfo_sets;
    } texture;

  } pipe;

  struct Frame {
    FlatSortedSet<vk::DescriptorSet> texture_diffuse_sets;
    vk::UniqueDescriptorPool diffuse_pool;
    std::vector<DrawFlatColor> drawflatcolor;
    std::vector<DrawTexture> drawtexture;
  };

  std::vector<Frame> frames;

private:
  RendererImpl() = default;
};

} // namespace bubble
