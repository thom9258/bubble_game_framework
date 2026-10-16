#include <bubble/bubble.hpp>
#include <bubble/timer.hpp>

#include <print>

auto vertices = std::vector<bubble::Vertex>{
    bubble::Vertex{},
    bubble::Vertex{},
    bubble::Vertex{},
};

auto indices = std::vector<bubble::Index>{1, 2, 3, 1};

int main(int argc, char **argv) {
  static_cast<void>(argc);
  static_cast<void>(argv);

  auto init_info = bubble::InitializeInfo{};
  bubble::Timer init_timer;
  init_timer.start();
  const auto ok = bubble::initialize(init_info);
  if (!ok) {
    return 1;
  }

  auto duration = init_timer.stop();
  if (duration.has_value()) {
    std::println("Initialized '{}' {}x{} in {}s", init_info.name,
                 init_info.width, init_info.height, duration.value().count());
  }

  auto mesh1 = bubble::create_mesh(vertices, indices);
  auto mesh2 = bubble::create_mesh(vertices, indices);
  if (!mesh1.has_value() || !mesh2.has_value()) {
    return 1;
  }

  std::println("Created mesh 2, index: {} gen: {}", mesh2->index(),
               mesh2->generation());

  auto bitmap = bubble::Bitmap(256, 256, bubble::Color{0, 255, 0, 255});
  auto texture = bubble::create_texture(bitmap);
  if (!texture.has_value()) {
    return 1;
  }

  bubble::set_clearcolor(bubble::Color{0, 0, 255, 255});

  while (!bubble::requested_close()) {
    bubble::begin_frame();
    bubble::draw_flatcolor(*mesh1, bubble::Color{0, 0, 255, 255});
    bubble::end_frame();
  }

  bubble::destroy();
  return 0;
}
