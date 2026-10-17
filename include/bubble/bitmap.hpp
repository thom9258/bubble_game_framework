#pragma once

#include <cstdint>
#include <vector>

namespace bubble {

struct Color {
  std::uint8_t r;
  std::uint8_t g;
  std::uint8_t b;
  std::uint8_t a;
};

namespace color {

inline constexpr const auto red = Color{255, 0, 0, 255};
inline constexpr const auto green = Color{0, 255, 0, 255};
inline constexpr const auto blue = Color{0, 0, 255, 255};
inline constexpr const auto black = Color{0, 0, 0, 255};
inline constexpr const auto white = Color{255, 255, 255, 255};
inline constexpr const auto ugly_pink = Color{254, 1, 177, 255};
inline constexpr const auto pleasant_blue = Color{56, 147, 193, 255};
} // namespace color

class Bitmap {
public:
  Bitmap(std::uint32_t width, std::uint32_t height);
  Bitmap(std::uint32_t width, std::uint32_t height, Color color);

  auto width() const -> std::uint32_t;
  auto height() const -> std::uint32_t;
  auto at(std::uint32_t x, std::uint32_t y) -> Color *;
  auto pixels() -> Color *;
  auto memory_size() const -> std::size_t;

private:
  std::uint32_t _width{0};
  std::uint32_t _height{0};
  std::vector<Color> _pixels;
};

} // namespace bubble
