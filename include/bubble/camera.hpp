#pragma once

#include "internal/glm.hpp"

namespace bubble
{

class Camera {
public:
  Camera(glm::vec3 position, glm::vec3 forward);

  auto translate_global(glm::vec3 delta) -> void;

  auto translate_local(glm::vec3 delta) -> void;

  auto set_position(glm::vec3 position) -> void;

  auto rotate(glm::vec2 delta) -> void;

  auto look_at(glm::vec3 target) -> void;

  [[nodiscard]]
  auto position() const -> glm::vec3;

  [[nodiscard]]
  auto forward() const -> glm::vec3;

  [[nodiscard]]
  auto up() const -> glm::vec3;

  [[nodiscard]]
  auto right() const -> glm::vec3;

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

}
