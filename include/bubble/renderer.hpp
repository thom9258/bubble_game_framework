#pragma once

#include <bubble/bitmap.hpp>
#include <bubble/camera.hpp>
#include <bubble/internal/glm.hpp>
#include <bubble/resources.hpp>
#include <bubble/system_concept.hpp>

namespace bubble {

struct HemisphereLight {
  Color color;
  Color sky;
  float intensity;
};

struct DirectionalLight {
  glm::vec3 direction;
  Color color;
  float intensity;
};

struct PointLight {
  glm::vec3 position;
  Color color;
  float intensity;
};

struct SpotLight {
  glm::vec3 position;
  glm::vec3 direction;
  Color color;
  float intensity;
};

class Renderer {
public:
  [[nodiscard]]
  static auto initialize(const InitializeConfig &config) -> bool;

  static auto destroy() -> void;

  static auto begin_frame() -> void;

  static auto end_frame() -> void;

  static auto set_clearcolor(Color color) -> void;

  static auto add_light(HemisphereLight light);

  static auto add_light(DirectionalLight light);

  static auto add_light(PointLight light);

  static auto add_light(SpotLight light);

  static auto set_camera(Camera camera) -> void;

  static auto draw_flatcolor(glm::mat4 location, MeshRef mesh, Color color)
      -> void;

  static auto draw_textured(glm::mat4 location, MeshRef mesh,
                            TextureRef texture) -> void;

  static auto deltatime() -> std::optional<double>;

private:
  Renderer() = delete;
};

} // namespace bubble

static_assert(bubble::System<bubble::Renderer>);
static_assert(bubble::SystemHasBeginFrame<bubble::Renderer>);
static_assert(bubble::SystemHasEndFrame<bubble::Renderer>);
