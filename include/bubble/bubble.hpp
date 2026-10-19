#pragma once

// TODO: we need to make a custom math wrapper at some point, but this is a
// cheat
#include "internal/glm.hpp"

#include "bitmap.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <string>

namespace bubble {

enum class LogType { info, warn, error };
using LogCallback = void (*)(LogType, std::string_view);

struct InitializeInfo {
  std::int32_t width{480};
  std::int32_t height{320};
  std::string name{"bubble"};

  struct {
    std::optional<LogCallback> log_callback;
    bool enable_graphics_validation{true};
    bool enable_vsync{true};
  } options;
};

[[nodiscard]]
auto initialize(InitializeInfo &info) -> bool;

auto destroy() -> void;

auto begin_drawing() -> void;

auto end_drawing() -> void;

[[nodiscard]]
auto requested_close() -> bool;

struct Vertex {
  float position[3];
  float normal[3];
  float texture_coordinate[2];
};

using Index = std::uint32_t;

class MeshRef {
public:
  MeshRef(std::uint32_t index, std::uint32_t generation);
  auto index() const -> std::uint32_t;
  auto generation() const -> std::uint32_t;

private:
  std::uint32_t _index;
  std::uint32_t _generation;
};

auto create_mesh(std::span<Vertex> vertices, std::span<Index> indices)
    -> std::optional<MeshRef>;

auto destroy_mesh(MeshRef ref) -> void;

class TextureRef {
public:
  TextureRef(std::uint32_t index, std::uint32_t generation);
  auto index() const -> std::uint32_t;
  auto generation() const -> std::uint32_t;

private:
  std::uint32_t _index;
  std::uint32_t _generation;
};

auto create_texture(Bitmap &bitmap) -> std::optional<TextureRef>;

auto destroy_texture(TextureRef ref) -> void;

auto set_clearcolor(Color color) -> void;

class Camera {
public:
  Camera(glm::vec3 position, glm::vec3 forward);

  auto translate_global(glm::vec3 delta) -> void;

  auto translate_local(glm::vec3 delta) -> void;

  auto rotate(glm::vec2 delta) -> void;

  auto look_at(glm::vec3 target) -> void;

  [[nodiscard]]
  auto position() const -> glm::vec3;

  [[nodiscard]]
  auto forward() const -> glm::vec3;

  [[nodiscard]]
  auto up() const -> glm::vec3;

  [[nodiscard]]
  auto fov_y() const -> float;

  [[nodiscard]]
  auto near() const -> float;

  [[nodiscard]]
  auto far() const -> float;

private:
  glm::vec3 _position{glm::vec3(0.0f, 0.0f, 0.0f)};
  glm::vec3 _forward{glm::vec3(0.0f, 0.0f, 1.0f)};
  glm::vec3 _up{glm::vec3(0.0f, 1.0f, 0.0f)};
  float _fov_y{glm::radians(45.0f)};
  float _near{0.1f};
  float _far{100.0f};
};

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

auto add_light(HemisphereLight light);

auto add_light(DirectionalLight light);

auto add_light(PointLight light);

auto add_light(SpotLight light);

auto set_camera(Camera camera) -> void;

auto begin_frame() -> void;

auto deltatime() -> std::optional<float>;

auto end_frame() -> void;

auto draw_flatcolor(glm::mat4 location, MeshRef mesh, Color color) -> void;

} // namespace bubble
