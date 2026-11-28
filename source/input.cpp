#include "input.hpp"
#include "bubble/internal/sdl.hpp"

#include <SDL_events.h>
#include <SDL_mouse.h>
#include <algorithm>
#include <cmath>
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

static_assert(std::to_underlying(GamepadButton::South) ==
              SDL_CONTROLLER_BUTTON_A);
static_assert(std::to_underlying(GamepadButton::North) ==
              SDL_CONTROLLER_BUTTON_Y);
static_assert(std::to_underlying(GamepadButton::Start) ==
              SDL_CONTROLLER_BUTTON_START);
static_assert(std::to_underlying(GamepadButton::RightShoulder) ==
              SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
static_assert(std::to_underlying(GamepadButton::DpadRight) ==
              SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
static_assert(std::to_underlying(GamepadAxis::LeftX) ==
              SDL_CONTROLLER_AXIS_LEFTX);
static_assert(std::to_underlying(GamepadAxis::RightY) ==
              SDL_CONTROLLER_AXIS_RIGHTY);
static_assert(std::to_underlying(GamepadAxis::RightTrigger) ==
              SDL_CONTROLLER_AXIS_TRIGGERRIGHT);

static std::unique_ptr<InputImpl> g_instance{nullptr};

auto InputImpl::instance() -> InputImpl & { return *g_instance; }

auto Input::destroy() -> void {
  if (g_instance == nullptr) {
    return;
  }

  for (auto &pad : g_instance->gamepads) {
    if (pad.handle != nullptr) {
      SDL_GameControllerClose(pad.handle);
    }
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

auto gamepads() -> std::array<GamepadState, max_gamepad_count> & {
  return InputImpl::instance().gamepads;
}

auto index(GamepadButton button) -> std::size_t {
  return static_cast<std::size_t>(std::to_underlying(button));
}

auto index(GamepadAxis axis) -> std::size_t {
  return static_cast<std::size_t>(std::to_underlying(axis));
}

// Public lookup by slot number; nullptr if out of range or nothing plugged in.
auto gamepad_at(int pad) -> GamepadState * {
  if (pad < 0 || static_cast<std::size_t>(pad) >= max_gamepad_count) {
    return nullptr;
  }
  auto &state = gamepads()[static_cast<std::size_t>(pad)];
  return state.handle != nullptr ? &state : nullptr;
}

// Event lookup: SDL events carry the joystick instance ID, not our slot.
auto gamepad_by_id(SDL_JoystickID id) -> GamepadState * {
  for (auto &pad : gamepads()) {
    if (pad.handle != nullptr && pad.instance_id == id) {
      return &pad;
    }
  }
  return nullptr;
}

auto release_all(GamepadState &pad) -> void {
  pad.released |= pad.down;
  pad.down.reset();
  pad.axes.fill(0.0f);
}

auto add_gamepad(int device_index) -> void {
  auto *handle = SDL_GameControllerOpen(device_index);
  if (handle == nullptr) {
    return;
  }

  const auto id = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(handle));

  // SDL can report a device we already hold; Open just bumped the refcount.
  if (gamepad_by_id(id) != nullptr) {
    SDL_GameControllerClose(handle);
    return;
  }

  for (auto &pad : gamepads()) {
    if (pad.handle == nullptr) {
      pad = GamepadState{};
      pad.handle = handle;
      pad.instance_id = id;
      return;
    }
  }

  // All slots taken.
  SDL_GameControllerClose(handle);
}

auto remove_gamepad(SDL_JoystickID id) -> void {
  auto *pad = gamepad_by_id(id);
  if (pad == nullptr) {
    return;
  }
  SDL_GameControllerClose(pad->handle);
  release_all(*pad); // held buttons report released() this frame
  pad->handle = nullptr;
  pad->instance_id = -1;
}

auto apply_deadzone(float value, float deadzone) -> float {
  const auto magnitude = std::abs(value);
  if (magnitude < deadzone) {
    return 0.0f;
  }
  return std::copysign((magnitude - deadzone) / (1.0f - deadzone), value);
}

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
      for (auto &pad : gamepads()) {
        release_all(pad);
      }
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
    if (!Input::mouse_locked()) {
      mouse_state().position.x = static_cast<float>(event.motion.x);
      mouse_state().position.y = static_cast<float>(event.motion.y);
    }

    mouse_state().delta.dx += static_cast<float>(event.motion.xrel);
    mouse_state().delta.dy += static_cast<float>(event.motion.yrel);
    break;
  }
  case SDL_MOUSEWHEEL: {
    float amount = event.wheel.preciseY;
    if (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED) {
      amount = -amount;
    }
    mouse_state().scroll += amount;
    break;
  }
  case SDL_CONTROLLERDEVICEADDED:
    add_gamepad(event.cdevice.which);
    break;
  case SDL_CONTROLLERDEVICEREMOVED:
    remove_gamepad(event.cdevice.which);
    break;
  case SDL_CONTROLLERBUTTONDOWN: {
    auto *pad = gamepad_by_id(event.cbutton.which);
    if (pad == nullptr || event.cbutton.button >= SDL_CONTROLLER_BUTTON_MAX) {
      break;
    }
    pad->down.set(event.cbutton.button);
    pad->pressed.set(event.cbutton.button);
    break;
  }
  case SDL_CONTROLLERBUTTONUP: {
    auto *pad = gamepad_by_id(event.cbutton.which);
    if (pad == nullptr || event.cbutton.button >= SDL_CONTROLLER_BUTTON_MAX) {
      break;
    }
    pad->down.reset(event.cbutton.button);
    pad->released.set(event.cbutton.button);
    break;
  }
  case SDL_CONTROLLERAXISMOTION: {
    auto *pad = gamepad_by_id(event.caxis.which);
    if (pad == nullptr || event.caxis.axis >= SDL_CONTROLLER_AXIS_MAX) {
      break;
    }
    const auto normalized =
        std::max(static_cast<float>(event.caxis.value) / 32767.0f, -1.0f);
    pad->axes[event.caxis.axis] = normalized;
    break;
  }
  default:
    break;
  }
}

} // namespace

auto Input::begin_frame() -> void {
  keyboard_state().pressed.reset();
  keyboard_state().released.reset();
  mouse_state().pressed.reset();
  mouse_state().released.reset();
  mouse_state().delta = MouseDelta{0.0f, 0.0f};
  mouse_state().scroll = 0.0f;
  for (auto &pad : gamepads()) {
    pad.pressed.reset();
    pad.released.reset();
  }

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
  if (locked == mouse_locked()) {
    return;
  }

  std::ignore = SDL_SetRelativeMouseMode(static_cast<SDL_bool>(locked));
  if (locked) {
    mouse_state().delta = MouseDelta{0.0f, 0.0f};
  }
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

auto Input::down(GamepadButton button, int pad) -> bool {
  const auto *state = gamepad_at(pad);
  return state != nullptr && state->down.test(index(button));
}

auto Input::pressed(GamepadButton button, int pad) -> bool {
  const auto *state = gamepad_at(pad);
  return state != nullptr && state->pressed.test(index(button));
}

auto Input::released(GamepadButton button, int pad) -> bool {
  // Not gamepad_at(): a pad unplugged this frame should still report releases.
  if (pad < 0 || static_cast<std::size_t>(pad) >= max_gamepad_count) {
    return false;
  }
  return gamepads()[static_cast<std::size_t>(pad)].released.test(index(button));
}

auto Input::axis(GamepadAxis axis, int pad) -> float {
  const auto *state = gamepad_at(pad);
  if (state == nullptr) {
    return 0.0f;
  }
  const auto is_trigger =
      axis == GamepadAxis::LeftTrigger || axis == GamepadAxis::RightTrigger;
  const auto deadzone = is_trigger ? 0.05f : 0.15f;
  return apply_deadzone(state->axes[index(axis)], deadzone);
}

auto Input::gamepad_connected(int pad) -> bool {
  return gamepad_at(pad) != nullptr;
}

} // namespace bubble
