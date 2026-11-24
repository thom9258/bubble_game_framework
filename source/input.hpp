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

class InputImpl {
public:
  friend Input;
  static auto instance() -> InputImpl &;
  KeyboardState keyboard_state;
  MouseState mouse_state;

private:
  InputImpl() = default;
};

} // namespace bubble
