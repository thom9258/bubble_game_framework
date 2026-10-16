#include <bubble/bitmap.hpp>

namespace bubble {

Bitmap::Bitmap(std::uint32_t width, std::uint32_t height)
    : _width{width}, _height{height} {
  _pixels.resize(width * height);
}

Bitmap::Bitmap(std::uint32_t width, std::uint32_t height, Color color)
    : _width{width}, _height{height} {
  _pixels.resize(width * height, color);
}

auto Bitmap::width() const -> std::uint32_t { return _width; }

auto Bitmap::height() const -> std::uint32_t { return _height; }

auto Bitmap::at(std::uint32_t x, std::uint32_t y) -> Color * {
  const auto index = (y * width()) + x;
  return &_pixels[index];
}

auto Bitmap::pixels() -> Color * { return _pixels.data(); }

auto Bitmap::memory_size() const -> std::size_t {
  return width() * height() * sizeof(Color);
}

} // namespace bubble
