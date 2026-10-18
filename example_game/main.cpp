
#include <bubble/bitmap.hpp>
#include <bubble/bubble.hpp>
#include <bubble/input.hpp>
#include <bubble/timer.hpp>

#include <bubble/internal/glm.hpp>
#include <bubble/internal/sdl.hpp>

#include <print>

std::pair<std::vector<bubble::Vertex>, std::vector<bubble::Index>>
generateIndexedCube() {

  std::vector<bubble::Vertex> vertices = {
      // --- FORSIDE (+Z) --- Normal: [0, 0, 1]
      {{-0.5f, -0.5f, 0.5f},
       {0.0f, 0.0f, 1.0f},
       {0.0f, 1.0f}}, // Nederst venstre
      {{0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}}, // Nederst højre
      {{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},  // Øverst højre
      {{-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}}, // Øverst venstre

      // --- BAGSIDE (-Z) --- Normal: [0, 0, -1]
      {{0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},
      {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},
      {{-0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},
      {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},

      // --- VENSTRE SIDE (-X) --- Normal: [-1, 0, 0]
      {{-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
      {{-0.5f, -0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
      {{-0.5f, 0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
      {{-0.5f, 0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},

      // --- HØJRE SIDE (+X) --- Normal: [1, 0, 0]
      {{0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
      {{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
      {{0.5f, 0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
      {{0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},

      // --- TOP (+Y) --- Normal: [0, 1, 0]
      {{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
      {{0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
      {{0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
      {{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},

      // --- BUND (-Y) --- Normal: [0, -1, 0]
      {{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
      {{0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
      {{0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
      {{-0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}}};

  // Index-rækkefølgen (Clockwise / Counter-Clockwise afhænger af dit Vulkan
  // FrontFace setup) Dette layout følger standard CCW (Counter-Clockwise)
  // winding order set udefra.
  std::vector<bubble::Index> indices = {
      0,  1,  2,  2,  3,  0,  // Forside
      4,  5,  6,  6,  7,  4,  // Bagside
      8,  9,  10, 10, 11, 8,  // Venstre
      12, 13, 14, 14, 15, 12, // Højre
      16, 17, 18, 18, 19, 16, // Top
      20, 21, 22, 22, 23, 20  // Bund
  };

  return {vertices, indices};
}

int main(int argc, char **argv) {
  static_cast<void>(argc);
  static_cast<void>(argv);

  bubble::Timer init_timer;
  init_timer.start();
  auto init_info = bubble::InitializeInfo{};
  init_info.height = 600;
  init_info.width = init_info.height * (4.0f / 3.0f);
  const auto ok = bubble::initialize(init_info);
  if (!ok) {
    return 1;
  }

  auto duration = init_timer.stop();
  if (duration.has_value()) {
    std::println("Initialized '{}' {}x{} in {}s", init_info.name,
                 init_info.width, init_info.height, duration.value().count());
  }

  auto [cube_vertices, cube_indices] = generateIndexedCube();
  auto cube_mesh = bubble::create_mesh(cube_vertices, cube_indices);
  if (!cube_mesh.has_value()) {
    return 1;
  }

  bubble::set_clearcolor(bubble::color::pleasant_blue);

  auto camera = bubble::Camera{};
  camera.position = glm::vec3(0.0f, 0.0f, -3.0f);
  camera.forward = glm::vec3(0.0f, 0.0f, 1.0f);
  camera.up = glm::vec3(0.0f, 1.0f, 0.0f);

  const auto cube_location = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f));
  const auto cube_color = bubble::color::ugly_pink;

  auto button_q = bubble::input::Button(SDLK_q);
  auto axis_x = bubble::input::Axis(SDLK_d, SDLK_a);
  auto axis_z = bubble::input::Axis(SDLK_s, SDLK_w);

  auto mouse = bubble::input::Mouse();

  auto should_run = true;
  while (should_run) {
    bubble::begin_frame();
    should_run = !bubble::requested_close();
    const auto deltatime = bubble::deltatime().value_or(0.16f);

    button_q.update();
    if (button_q.pressed()) {
      should_run = false;
    }

    axis_x.update();
    axis_z.update();
    const auto move_direction =
        glm::normalize(glm::vec3(axis_x.value(), 0.0f, axis_z.value()));
    if (glm::length(move_direction) > 0.1f) {
      const auto move_speed = 2.0f;
      camera.position += move_direction * deltatime * move_speed;
    }

    mouse.update();
    std::println("mouse pos: {}, mouse delta: {} mouse locked: {}",
                 glm::to_string(mouse.position()),
                 glm::to_string(mouse.move_delta()),
                 mouse.locked());
    if (mouse.left_pressed()) {
        mouse.set_locked(true);
    }
    if (mouse.right_pressed()) {
        mouse.set_locked(false);
    }

    bubble::set_camera(camera);
    bubble::draw_flatcolor(cube_location, *cube_mesh, cube_color);
    bubble::end_frame();
  }

  bubble::destroy();
  return 0;
}
