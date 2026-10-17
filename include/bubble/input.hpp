#pragma once

// TODO: should be hidden beneath...
#include "internal/sdl.hpp"

namespace bubble::input {

class Button {
public:
  explicit Button(int keycode);
  auto update() -> void;
  auto pressed() const -> bool;
  auto released() const -> bool;
  auto held() const -> bool;

private:
  int _keycode{0};
  bool _pressed{false};
  bool _released{false};
  bool _held{false};
};

class Axis {
public:
  explicit Axis(int negative, int positive);
  auto update() -> void;
  auto value() const -> int;

private:
  int _negative{0};
  int _positive{0};
  bool _negative_held{false};
  bool _positive_held{false};
};



} // namespace bubble
