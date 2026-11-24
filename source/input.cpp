#include "input.hpp"
#include "bubble/internal/sdl.hpp"

#include <SDL_events.h>
#include <SDL_mouse.h>
#include <memory>
#include <utility>

namespace bubble {

static_assert(std::to_underlying(Key::Unknown) == SDL_SCANCODE_UNKNOWN);
static_assert(std::to_underlying(Key::A) == SDL_SCANCODE_A);
static_assert(std::to_underlying(Key::Z) == SDL_SCANCODE_Z);
static_assert(std::to_underlying(Key::Num1) == SDL_SCANCODE_1);
static_assert(std::to_underlying(Key::Num9) == SDL_SCANCODE_9);
static_assert(std::to_underlying(Key::Num0) == SDL_SCANCODE_0);
static_assert(std::to_underlying(Key::Enter) == SDL_SCANCODE_RETURN);
static_assert(std::to_underlying(Key::Space) == SDL_SCANCODE_SPACE);
static_assert(std::to_underlying(Key::Minus) == SDL_SCANCODE_MINUS);
static_assert(std::to_underlying(Key::Backslash) == SDL_SCANCODE_BACKSLASH);
static_assert(std::to_underlying(Key::Semicolon) == SDL_SCANCODE_SEMICOLON);
static_assert(std::to_underlying(Key::Slash) == SDL_SCANCODE_SLASH);
static_assert(std::to_underlying(Key::CapsLock) == SDL_SCANCODE_CAPSLOCK);
static_assert(std::to_underlying(Key::F1) == SDL_SCANCODE_F1);
static_assert(std::to_underlying(Key::F12) == SDL_SCANCODE_F12);
static_assert(std::to_underlying(Key::PrintScreen) == SDL_SCANCODE_PRINTSCREEN);
static_assert(std::to_underlying(Key::Pause) == SDL_SCANCODE_PAUSE);
static_assert(std::to_underlying(Key::Insert) == SDL_SCANCODE_INSERT);
static_assert(std::to_underlying(Key::PageDown) == SDL_SCANCODE_PAGEDOWN);
static_assert(std::to_underlying(Key::Right) == SDL_SCANCODE_RIGHT);
static_assert(std::to_underlying(Key::Up) == SDL_SCANCODE_UP);
static_assert(std::to_underlying(Key::NumLock) == SDL_SCANCODE_NUMLOCKCLEAR);
static_assert(std::to_underlying(Key::KeypadDivide) == SDL_SCANCODE_KP_DIVIDE);
static_assert(std::to_underlying(Key::KeypadEnter) == SDL_SCANCODE_KP_ENTER);
static_assert(std::to_underlying(Key::Keypad1) == SDL_SCANCODE_KP_1);
static_assert(std::to_underlying(Key::KeypadPeriod) == SDL_SCANCODE_KP_PERIOD);
static_assert(std::to_underlying(Key::LeftCtrl) == SDL_SCANCODE_LCTRL);
static_assert(std::to_underlying(Key::LeftSuper) == SDL_SCANCODE_LGUI);
static_assert(std::to_underlying(Key::RightCtrl) == SDL_SCANCODE_RCTRL);
static_assert(std::to_underlying(Key::RightSuper) == SDL_SCANCODE_RGUI);

static std::unique_ptr<InputImpl> g_instance{nullptr};

auto InputImpl::instance() -> InputImpl & { return *g_instance; }

auto Input::destroy() -> void {
  if (g_instance == nullptr) {
    return;
  }

  g_instance.reset();
}

auto Input::initialize(const InitializeConfig &) -> bool {
  if (g_instance != nullptr) {
    destroy();
  }

  g_instance.reset(new InputImpl);

  return true;
}

namespace {

auto keyboard_state() -> KeyboardState & {
  return InputImpl::instance().keyboard_state;
}

auto mouse_state() -> MouseState & { return InputImpl::instance().mouse_state; }

auto index(Key key) -> std::size_t {
  return static_cast<std::size_t>(std::to_underlying(key));
}

auto index(MouseButton button) -> std::size_t {
  return static_cast<std::size_t>(std::to_underlying(button));
}

} // namespace

auto handle_input_event(const SDL_Event &event) -> void {
  switch (event.type) {
  case SDL_KEYDOWN: {
    if (event.key.repeat != 0) {
      break;
    }
    const auto scancode = static_cast<std::size_t>(event.key.keysym.scancode);
    if (scancode >= SDL_NUM_SCANCODES) {
      break;
    }
    keyboard_state().down.set(scancode);
    keyboard_state().pressed.set(scancode);
    break;
  }
  case SDL_KEYUP: {
    const auto scancode = static_cast<std::size_t>(event.key.keysym.scancode);
    if (scancode >= SDL_NUM_SCANCODES) {
      break;
    }

    keyboard_state().down.reset(scancode);
    keyboard_state().released.set(scancode);
    break;
  }
  case SDL_WINDOWEVENT:
    if (event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
      keyboard_state().released |= keyboard_state().down;
      keyboard_state().down.reset();
      mouse_state().released |= mouse_state().down;
      mouse_state().down.reset();
    }
    break;
  case SDL_MOUSEBUTTONDOWN: {
    const auto button = static_cast<std::size_t>(event.button.button);
    if (button >= mouse_state().down.size()) {
      break;
    }
    mouse_state().down.set(button);
    mouse_state().pressed.set(button);
    break;
  }
  case SDL_MOUSEBUTTONUP: {
    const auto button = static_cast<std::size_t>(event.button.button);
    if (button >= mouse_state().down.size()) {
      break;
    }
    mouse_state().down.reset(button);
    mouse_state().released.set(button);
    break;
  }
  case SDL_MOUSEMOTION: {
    mouse_state().position.x = static_cast<float>(event.motion.x);
    mouse_state().position.y = static_cast<float>(event.motion.y);
    mouse_state().delta.dx += static_cast<float>(event.motion.xrel);
    mouse_state().delta.dy += static_cast<float>(event.motion.yrel);
    break;
  }
  case SDL_MOUSEWHEEL: {
    float amount = event.wheel.preciseY; // SDL >= 2.0.18, else event.wheel.y
    if (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED) {
      amount = -amount;
    }
    mouse_state().scroll += amount;
    break;
  }
  default:
    break;
  }
}

auto Input::begin_frame() -> void {
  keyboard_state().pressed.reset();
  keyboard_state().released.reset();
  mouse_state().pressed.reset();
  mouse_state().released.reset();
  mouse_state().delta = MouseDelta{0.0f, 0.0f};
  mouse_state().position = MousePosition{0.0f, 0.0f};

  for (const auto &event : get_sdl_events()) {
    handle_input_event(event);
  }
}

auto Input::down(Key key) -> bool {
  return keyboard_state().down.test(index(key));
}

auto Input::pressed(Key key) -> bool {
  return keyboard_state().pressed.test(index(key));
}

auto Input::released(Key key) -> bool {
  return keyboard_state().released.test(index(key));
}

auto Input::axis(Key negative, Key positive) -> float {
  return static_cast<float>(down(positive)) -
         static_cast<float>(down(negative));
}

auto Input::shift() -> bool {
  return down(Key::LeftShift) || down(Key::RightShift);
}

auto Input::ctrl() -> bool {
  return down(Key::LeftCtrl) || down(Key::RightCtrl);
}

auto Input::alt() -> bool { return down(Key::LeftAlt) || down(Key::RightAlt); }

auto Input::down(MouseButton button) -> bool {
  return mouse_state().down.test(index(button));
}

auto Input::pressed(MouseButton button) -> bool {
  return mouse_state().pressed.test(index(button));
}

auto Input::released(MouseButton button) -> bool {
  return mouse_state().released.test(index(button));
}

[[nodiscard]] auto Input::mouse_position() -> MousePosition {
  return mouse_state().position;
}

[[nodiscard]] auto Input::mouse_delta() -> MouseDelta {
  return mouse_state().delta;
}

auto Input::mouse_set_locked(bool locked) -> void {
  SDL_SetRelativeMouseMode(static_cast<SDL_bool>(locked));
}

[[nodiscard]] auto Input::mouse_locked() -> bool {
  return SDL_GetRelativeMouseMode();
}

auto Input::mouse_set_visible(bool visible) -> void {
  SDL_ShowCursor(visible ? SDL_ENABLE : SDL_DISABLE);
}

[[nodiscard]] auto Input::mouse_visible() -> bool {
  return SDL_ShowCursor(SDL_QUERY) == SDL_ENABLE;
}

} // namespace bubble
