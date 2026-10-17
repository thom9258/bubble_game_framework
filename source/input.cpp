#include <bubble/input.hpp>

#include "bubble/internal/sdl.hpp"

namespace bubble::input {
Button::Button(int keycode) : _keycode{keycode} {}

auto Button::update() -> void {
  _pressed = false;
  _released = false;
  for (const auto &event : get_sdl_events()) {

    if (event.type == SDL_KEYDOWN) {
      if (event.key.keysym.sym == _keycode) {
        if (event.key.repeat == 0) {
          _pressed = true;
          _held = true;
        }
      }
    }

    if (event.type == SDL_KEYUP) {
      if (event.key.keysym.sym == _keycode) {
        _released = true;
        _held = false;
      }
    }
  }
}

auto Button::pressed() const -> bool { return _pressed; }
auto Button::released() const -> bool { return _released; }
auto Button::held() const -> bool { return _held; }

Axis::Axis(int negative_keycode, int positive_keycode)
    : _negative(negative_keycode), _positive(positive_keycode) {}

auto Axis::update() -> void {
  for (const auto &event : get_sdl_events()) {
    if (event.type == SDL_KEYDOWN && event.key.keysym.sym == _negative) {
      _negative_held = true;
    } else if (event.type == SDL_KEYUP && event.key.keysym.sym == _negative) {
      _negative_held = false;
    }

    // Tjek den positive tast (f.eks. Højre / D / Op)
    if (event.type == SDL_KEYDOWN && event.key.keysym.sym == _positive) {
      _positive_held = true;
    } else if (event.type == SDL_KEYUP && event.key.keysym.sym == _positive) {
      _positive_held = false;
    }
  }
}

auto Axis::value() const -> int {
  return static_cast<int>(_positive_held) - static_cast<int>(_negative_held);
}

} // namespace bubble::input
