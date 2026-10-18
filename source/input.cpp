#include <bubble/input.hpp>

#include "context.hpp"

namespace bubble::input {

auto ButtonState::next_frame() -> void {
  _pressed = false;
  _released = false;
}

auto ButtonState::press() -> void {
  _pressed = true;
  _held = true;
}

auto ButtonState::release() -> void {
  _released = true;
  _held = false;
}

auto ButtonState::pressed() const -> bool { return _pressed; }

auto ButtonState::released() const -> bool { return _released; }

auto ButtonState::held() const -> bool { return _held; }

Button::Button(int keycode) : _keycode{keycode} {}

auto Button::update() -> void {
  _state.next_frame();
  for (const auto &event : get_sdl_events()) {
    if (event.type == SDL_KEYDOWN && event.key.keysym.sym == _keycode) {
      if (event.key.repeat == 0) {
        _state.press();
      }
    }

    if (event.type == SDL_KEYUP && event.key.keysym.sym == _keycode) {
      _state.release();
    }
  }
}

auto Button::pressed() const -> bool { return _state.pressed(); }

auto Button::released() const -> bool { return _state.released(); }

auto Button::held() const -> bool { return _state.held(); }

Axis::Axis(int negative_keycode, int positive_keycode)
    : _negative(negative_keycode), _positive(positive_keycode) {}

auto Axis::update() -> void {
  for (const auto &event : get_sdl_events()) {
    if (event.type == SDL_KEYDOWN && event.key.keysym.sym == _negative) {
      _negative_held = true;
    } else if (event.type == SDL_KEYUP && event.key.keysym.sym == _negative) {
      _negative_held = false;
    }

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

auto Mouse::update() -> void {
  _move_delta = glm::ivec2(0, 0);
  _left.next_frame();
  _right.next_frame();
  for (const auto &event : get_sdl_events()) {
    if (event.type == SDL_MOUSEMOTION) {
      _position = glm::ivec2(event.motion.x, event.motion.y);
      _move_delta += glm::ivec2(event.motion.xrel, event.motion.yrel);
    } else if (event.type == SDL_MOUSEBUTTONDOWN) {
      if (event.button.button == SDL_BUTTON_LEFT) {
        _left.press();
      } else if (event.button.button == SDL_BUTTON_RIGHT) {
        _right.press();
      }
    }
    // Håndter slip af klik
    else if (event.type == SDL_MOUSEBUTTONUP) {
      if (event.button.button == SDL_BUTTON_LEFT) {
        _left.release();
      } else if (event.button.button == SDL_BUTTON_RIGHT) {
        _right.release();
      }
    }
  }

  if (locked()) {
    center_mouse_position();
  }
}

auto Mouse::center_mouse_position() -> void {
  int width, height;
  SDL_GetWindowSize(context::instance().window.get(), &width, &height);
  _position = glm::ivec2(width / 2, height / 2);
  SDL_WarpMouseInWindow(context::instance().window.get(), _position.x, _position.y);
}

auto Mouse::position() -> glm::ivec2 { return _position; }

auto Mouse::move_delta() -> glm::ivec2 { return _move_delta; }

auto Mouse::left_pressed() const -> bool { return _left.pressed(); }

auto Mouse::left_released() const -> bool { return _left.released(); }

auto Mouse::left_held() const -> bool { return _left.held(); }

auto Mouse::right_pressed() const -> bool { return _right.pressed(); }

auto Mouse::right_released() const -> bool { return _right.released(); }

auto Mouse::right_held() const -> bool { return _right.held(); }

auto Mouse::lock() -> void {
  _locked = true;
  SDL_SetRelativeMouseMode(SDL_TRUE);
}

auto Mouse::unlock() -> void {
  _locked = false;
  SDL_SetRelativeMouseMode(SDL_FALSE);
}

auto Mouse::locked() const -> bool { return _locked; }

} // namespace bubble::input
