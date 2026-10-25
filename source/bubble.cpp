#include "../include/bubble/bubble.hpp"

#include "context.hpp"
#include "renderer.hpp"
#include "resources.hpp"

namespace bubble {

auto initialize(InitializeInfo &info) -> bool {

  if (!context::initialize(info)) {
    return false;
  }

  if (!renderer::initialize(info)) {
    return false;
  }

  if (!resources::initialize()) {
    return false;
  }

  return true;
}

auto destroy() -> void {
  context::device().waitIdle();
  resources::destroy();
  renderer::destroy();
  context::destroy();
}

auto begin_frame() -> void {
  context::begin_frame();
  renderer::begin_frame();
}

auto set_clearcolor(Color color) -> void { renderer::set_clearcolor(color); }

auto set_camera(Camera camera) -> void { renderer::set_camera(camera); }

auto requested_close() -> bool { return context::requested_close(); }

auto end_frame() -> void { renderer::end_frame(); }

auto draw_flatcolor(glm::mat4 location, MeshRef mesh, Color color) -> void {
  renderer::draw_flatcolor(location, mesh, color);
}

auto draw_textured(glm::mat4 location, MeshRef mesh, TextureRef texture) -> void {
  renderer::draw_textured(location, mesh, texture);
}

auto deltatime() -> std::optional<float> {
  return renderer::instance().deltatime;
}

Camera::Camera(glm::vec3 position, glm::vec3 forward)
    : _position{position}, _forward{forward} {}

auto Camera::translate_global(glm::vec3 delta) -> void { _position += delta; }

auto Camera::translate_local(glm::vec3 delta) -> void {
  const auto right = glm::normalize(glm::cross(_up, _forward));
  const auto displacement =
      (right * delta.x) + (_up * delta.y) + (_forward * delta.z);
  _position += displacement;
}

auto Camera::rotate(glm::vec2 delta) -> void {
  auto yaw = std::atan2(_forward.z, _forward.x);
  auto pitch = std::asin(glm::clamp(_forward.y, -0.999f, 0.999f));
  yaw += delta.x;
  pitch -= delta.y;
  const auto max_pitch = glm::radians(89.0f);
  pitch = glm::clamp(pitch, -max_pitch, max_pitch);
  const auto new_forward =
      glm::vec3(std::cos(pitch) * std::cos(yaw), std::sin(pitch),
                std::cos(pitch) * std::sin(yaw));
  _forward = glm::normalize(new_forward);
}

auto Camera::look_at(glm::vec3 target) -> void {
  const auto direction = target - _position;
  if (glm::length(direction) > 0.0001f) {
    _forward = glm::normalize(direction);
  }
}

auto Camera::position() const -> glm::vec3 { return _position; }

auto Camera::forward() const -> glm::vec3 { return _forward; }

auto Camera::up() const -> glm::vec3 { return _up; }

auto Camera::fov_y() const -> float { return _fov_y; }

auto Camera::near() const -> float { return _near; }

auto Camera::far() const -> float { return _far; }

} // namespace bubble
