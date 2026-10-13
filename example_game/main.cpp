#include <bubble/bubble.hpp>
#include <bubble/timer.hpp>

#include <print>

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

  auto vertices = std::vector<bubble::Vertex>{
      bubble::Vertex{},
      bubble::Vertex{},
      bubble::Vertex{},
  };

  auto indices = std::vector<bubble::Index>{
      1,2,3,1
  };

  auto mesh_ref1 = bubble::create_mesh(vertices, indices);
  auto mesh_ref2 = bubble::create_mesh(vertices, indices);
  if (!mesh_ref2.has_value()) {
    return 1;
  }

  std::println("Created mesh 2 index: {} gen: {}", mesh_ref2->index(),
               mesh_ref2->generation());

  bool should_close{false};
  while (!should_close) {
    bubble::next_frame();
    should_close = bubble::requested_close();
  }

  bubble::destroy_mesh(mesh_ref1.value());
  bubble::destroy_mesh(mesh_ref2.value());
  bubble::destroy();
  return 0;
}
