#include "bubble/bitmap.hpp"
#include <SDL_events.h>
#include <bubble/bubble.hpp>
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
  camera.target = glm::vec3(0.0f, 0.0f, 0.0f);
  camera.up = glm::vec3(0.0f, 1.0f, 3.0f);

  const auto cube_location = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f));
  const auto cube_color = bubble::color::ugly_pink;

  auto strafe_direction = glm::vec3{0.0f};

  while (!bubble::requested_close()) {
    bubble::begin_frame();

    const auto events = bubble::get_sdl_events();
    for (auto &event : events) {
      if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
        SDL_Keycode key = event.key.keysym.sym;
        if (key == SDLK_a && event.type == SDL_KEYDOWN) {
          strafe_direction.x -= 1.0f;
        }
        if (key == SDLK_a && event.type == SDL_KEYUP) {
          strafe_direction.x += 1.0f;
        }
        if (key == SDLK_d && event.type == SDL_KEYDOWN) {
          strafe_direction.x += 1.0f;
        }
        if (key == SDLK_d && event.type == SDL_KEYUP) {
          strafe_direction.x -= 1.0f;
        }
      }
    }

    camera.position += glm::normalize(strafe_direction * bubble::deltatime());
     
    bubble::set_camera(camera);
    bubble::draw_flatcolor(cube_location, *cube_mesh, cube_color);
    bubble::end_frame();
  }

  bubble::destroy();
  return 0;
}
