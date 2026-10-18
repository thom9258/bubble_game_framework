#pragma once

// TODO: should be hidden beneath...
#include "internal/glm.hpp"
#include "internal/sdl.hpp"

namespace bubble::input {

class ButtonState {
public:
  auto next_frame() -> void;

  auto press() -> void;

  auto release() -> void;

  [[nodiscard]]
  auto pressed() const -> bool;

  [[nodiscard]]
  auto released() const -> bool;

  [[nodiscard]]
  auto held() const -> bool;

private:
  bool _pressed{false};
  bool _released{false};
  bool _held{false};
};

class Button {
public:
  explicit Button(int keycode);

  auto update() -> void;

  [[nodiscard]]
  auto pressed() const -> bool;

  [[nodiscard]]
  auto released() const -> bool;

  [[nodiscard]]
  auto held() const -> bool;

private:
  int _keycode{0};
  ButtonState _state;
};

class Axis {
public:
  explicit Axis(int negative, int positive);

  auto update() -> void;

  [[nodiscard]]
  auto value() const -> int;

private:
  int _negative{0};
  int _positive{0};
  bool _negative_held{false};
  bool _positive_held{false};
};

class Mouse {
public:
  auto update() -> void;

  [[nodiscard]]
  auto position() -> glm::ivec2;

  [[nodiscard]]
  auto move_delta() -> glm::ivec2;

  [[nodiscard]]
  auto left_pressed() const -> bool;

  [[nodiscard]]
  auto left_released() const -> bool;

  [[nodiscard]]
  auto left_held() const -> bool;

  [[nodiscard]]
  auto right_pressed() const -> bool;

  [[nodiscard]]
  auto right_released() const -> bool;

  [[nodiscard]]
  auto right_held() const -> bool;

// auto enable_hide_and_lock() -> void;
//
// auto disable_hide_and_lock() -> void;

private:
  bool _hide_and_lock{false};
  glm::ivec2 _move_delta{};
  glm::ivec2 _position{};
  ButtonState _left;
  ButtonState _right;
};

} // namespace bubble::input
