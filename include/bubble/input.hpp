#pragma once

#include "imgui.h"
#include <bubble/system_concept.hpp>

#include <cstdint>

namespace bubble {

enum class Key : std::uint16_t {
  Unknown = 0,

  A = 4,
  B,
  C,
  D,
  E,
  F,
  G,
  H,
  I,
  J,
  K,
  L,
  M,
  N,
  O,
  P,
  Q,
  R,
  S,
  T,
  U,
  V,
  W,
  X,
  Y,
  Z,

  Num1 = 30,
  Num2,
  Num3,
  Num4,
  Num5,
  Num6,
  Num7,
  Num8,
  Num9,
  Num0 = 39,

  Enter = 40,
  Escape,
  Backspace,
  Tab,
  Space,
  Minus = 45,
  Equals,
  LeftBracket,
  RightBracket,
  Backslash,
  Semicolon = 51,
  Apostrophe,
  Grave,
  Comma,
  Period,
  Slash,
  CapsLock = 57,

  F1 = 58,
  F2,
  F3,
  F4,
  F5,
  F6,
  F7,
  F8,
  F9,
  F10,
  F11,
  F12,

  PrintScreen = 70,
  ScrollLock,
  Pause,
  Insert = 73,
  Home,
  PageUp,
  Delete,
  End,
  PageDown,
  Right = 79,
  Left,
  Down,
  Up,

  NumLock = 83,
  KeypadDivide = 84,
  KeypadMultiply,
  KeypadMinus,
  KeypadPlus,
  KeypadEnter,
  Keypad1 = 89,
  Keypad2,
  Keypad3,
  Keypad4,
  Keypad5,
  Keypad6,
  Keypad7,
  Keypad8,
  Keypad9,
  Keypad0,
  KeypadPeriod,

  LeftCtrl = 224,
  LeftShift,
  LeftAlt,
  LeftSuper,
  RightCtrl = 228,
  RightShift,
  RightAlt,
  RightSuper
};

enum class MouseButton : std::uint8_t {
  Left = 1,
  Middle,
  Right,
  Back,
  Forward
};

struct MousePosition {
  float x{0};
  float y{0};
};

struct MouseDelta {
  float dx{0};
  float dy{0};
};

// Values match SDL_GameControllerButton. Positional names, because SDL's
// A/B/X/Y means the Xbox *position*, which is wrong on Nintendo pads.
enum class GamepadButton : std::uint8_t {
  South = 0,
  East,
  West,
  North, // SDL_CONTROLLER_BUTTON_A, B, X, Y
  Back,
  Guide,
  Start,
  LeftStick,
  RightStick,
  LeftShoulder,
  RightShoulder,
  DpadUp,
  DpadDown,
  DpadLeft,
  DpadRight,
};

// Values match SDL_GameControllerAxis.
enum class GamepadAxis : std::uint8_t {
  LeftX = 0,
  LeftY,
  RightX,
  RightY,
  LeftTrigger,
  RightTrigger,
};

class Input {
public:
  [[nodiscard]]
  static auto initialize(const InitializeConfig &config) -> bool;

  static auto destroy() -> void;

  static auto begin_frame() -> void;

  [[nodiscard]] static auto down(Key key) -> bool;
  [[nodiscard]] static auto pressed(Key key) -> bool;
  [[nodiscard]] static auto released(Key key) -> bool;

  [[nodiscard]] static auto axis(Key negative, Key positive) -> float;

  [[nodiscard]] static auto shift() -> bool;
  [[nodiscard]] static auto ctrl() -> bool;
  [[nodiscard]] static auto alt() -> bool;

  [[nodiscard]] static auto down(MouseButton button) -> bool;
  [[nodiscard]] static auto pressed(MouseButton button) -> bool;
  [[nodiscard]] static auto released(MouseButton button) -> bool;

  [[nodiscard]] static auto mouse_position() -> MousePosition;
  [[nodiscard]] static auto mouse_delta() -> MouseDelta;

  static auto mouse_set_locked(bool locked) -> void;
  [[nodiscard]] static auto mouse_locked() -> bool;

  static auto mouse_set_visible(bool visible) -> void;
  [[nodiscard]] static auto mouse_visible() -> bool;

  [[nodiscard]] static auto down(GamepadButton button, int pad = 0) -> bool;
  [[nodiscard]] static auto pressed(GamepadButton button, int pad = 0) -> bool;
  [[nodiscard]] static auto released(GamepadButton button, int pad = 0) -> bool;

  [[nodiscard]] static auto axis(GamepadAxis axis, int pad = 0) -> float;

  [[nodiscard]] static auto gamepad_connected(int pad = 0) -> bool;

private:
  Input() = delete;
};

} // namespace bubble

static_assert(bubble::System<bubble::Input>);
static_assert(bubble::SystemHasBeginFrame<bubble::Input>);
