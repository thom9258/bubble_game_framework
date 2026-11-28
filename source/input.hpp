#include <bubble/input.hpp>

#include <bubble/internal/sdl.hpp>

#include <bitset>

namespace bubble {

struct KeyboardState {
  std::bitset<SDL_NUM_SCANCODES> down;
  std::bitset<SDL_NUM_SCANCODES> pressed;
  std::bitset<SDL_NUM_SCANCODES> released;
};

struct MouseState {
  std::bitset<8> down;
  std::bitset<8> pressed;
  std::bitset<8> released;
  MousePosition position{};
  MouseDelta delta{};
  float scroll{0.0f};
};

static constexpr const auto max_gamepad_count = 4u;
struct GamepadState {
  SDL_GameController *handle{nullptr};
  SDL_JoystickID instance_id{-1};
  std::bitset<SDL_CONTROLLER_BUTTON_MAX> down, pressed, released;
  std::array<float, SDL_CONTROLLER_AXIS_MAX> axes{};
};

class InputImpl {
public:
  friend Input;
  static auto instance() -> InputImpl &;
  KeyboardState keyboard_state;
  MouseState mouse_state;
  std::array<GamepadState, max_gamepad_count> gamepads;

private:
  InputImpl() = default;
};

} // namespace bubble
