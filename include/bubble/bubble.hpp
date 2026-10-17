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

struct Camera {
  glm::vec3 position;
  glm::vec3 forward;
  glm::vec3 up;
  float fov_y{glm::radians(45.0f)};
  float near{0.1f};
  float far{100.0f};
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
