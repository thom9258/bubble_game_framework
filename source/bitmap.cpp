#include <bubble/bitmap.hpp>
#include <cstdint>

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#endif

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STBI_FAILURE_USERMSG
#include <stb_image.h>

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

#include <cstring>
#include <filesystem>
#include <memory>

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

struct STBDeleter {
  auto operator()(stbi_uc *p) const noexcept -> void { stbi_image_free(p); }
};

using STBPixels = std::unique_ptr<stbi_uc, STBDeleter>;

auto load_bitmap(std::filesystem::path path)
    -> std::expected<Bitmap, std::string> {
  if (!std::filesystem::is_regular_file(path)) {
    return std::unexpected(std::format("No image at path '{}'", path.string()));
  }

  stbi_set_flip_vertically_on_load(false);

  auto width = 0;
  auto height = 0;
  auto channels = 0;
  auto pixels = STBPixels(
      stbi_load(path.string().c_str(), &width, &height, &channels, 4));

  if (pixels == nullptr) {
    return std::unexpected(std::format("failed to load '{}': {}", path.string(),
                                       stbi_failure_reason()));
  }

  auto bitmap = bubble::Bitmap(static_cast<std::uint32_t>(width),
                               static_cast<std::uint32_t>(height));
  std::memcpy(bitmap.pixels(), pixels.get(),
              static_cast<std::size_t>(width * height * 4));

  return bitmap;
}

} // namespace bubble
