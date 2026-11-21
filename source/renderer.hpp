#pragma once

#include <bubble/internal/glm.hpp>
#include <bubble/internal/imgui.hpp>
#include <bubble/timer.hpp>

#include "bubble/animation.hpp"
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

struct DrawAnimation {
  glm::mat4 location;
  AnimatedMeshRef mesh;
  TextureRef diffuse;
  std::vector<glm::mat4> bone_matrices;
};

struct FlatColorUniform {
  glm::mat4 model;
  glm::mat4 view;
  glm::mat4 projection;
  alignas(sizeof(glm::vec4)) glm::vec3 color;
};

struct HemisphereLightUniform {
  alignas(sizeof(glm::vec4)) glm::vec3 color;
  alignas(sizeof(glm::vec4)) glm::vec3 sky;
  float intensity;
};

struct DirectionalLightUniform {
  alignas(sizeof(glm::vec4)) glm::vec3 direction;
  alignas(sizeof(glm::vec4)) glm::vec3 color;
  float intensity;
};

struct PointLightUniform {
  alignas(sizeof(glm::vec4)) glm::vec3 position;
  alignas(sizeof(glm::vec4)) glm::vec3 color;
  float intensity;
};

struct TextureUniform {
  glm::mat4 model;
  glm::mat4 view;
  glm::mat4 projection;

  HemisphereLightUniform hemisphere_light;
  DirectionalLightUniform directional_light;
  PointLightUniform point_lights[4];
};

inline constexpr const auto max_animation_bones = std::size_t{100};

struct AnimatedUniform {
  glm::mat4 model;
  glm::mat4 view;
  glm::mat4 projection;

  HemisphereLightUniform hemisphere_light;
  DirectionalLightUniform directional_light;
  PointLightUniform point_lights[4];

  glm::mat4 animation_bones[max_animation_bones];
};

class RendererImpl {
public:
  friend Renderer;
  static auto instance() -> RendererImpl &;

  Timer deltatime_timer;
  std::optional<double> deltatime;
  Color clearcolor{255, 0, 0, 255};
  Camera camera{glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f)};
  bool swapchain_dirty{false};
  bool frame_acquired{false};

  std::vector<TaskGraph> rendergraphs;
  std::vector<UniformArena> uniform_arenas;

  vk::SurfaceFormatKHR swapchain_format;
  vk::UniqueSwapchainKHR swapchain;
  vk::Extent2D swapchain_extent;
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

    struct {
      vk::UniquePipelineLayout layout;
      vk::UniquePipeline pipeline;
      vk::UniqueDescriptorSetLayout modelinfo_layout;
      vk::UniqueDescriptorSetLayout diffuse_layout;
      std::vector<vk::DescriptorSet> modelinfo_sets;
    } animation;

    struct {
      vk::UniqueDescriptorPool pool;
      float ui_scale{1.0f};
    } imgui;

  } pipe;

  struct Frame {
    FlatSortedSet<vk::DescriptorSet> texture_diffuse_sets;
    FlatSortedSet<vk::DescriptorSet> animation_diffuse_sets;
    vk::UniqueDescriptorPool diffuse_pool;
    std::vector<DrawFlatColor> drawflatcolor;
    std::vector<DrawTexture> drawtexture;
    std::vector<DrawAnimation> drawanimation;
    std::vector<HemisphereLight> hemisphere_lights;
    std::vector<DirectionalLight> directional_lights;
    std::vector<PointLight> point_lights;
  };

  std::vector<Frame> frames;

private:
  RendererImpl() = default;
};

} // namespace bubble
