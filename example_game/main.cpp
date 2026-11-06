#include <bubble/bitmap.hpp>
#include <bubble/bubble.hpp>
#include <bubble/context.hpp>
#include <bubble/input.hpp>
#include <bubble/renderer.hpp>
#include <bubble/resources.hpp>
#include <bubble/timer.hpp>

#include <bubble/internal/glm.hpp>
#include <bubble/internal/sdl.hpp>

#include <print>

int main(int argc, char **argv) {
  static_cast<void>(argc);
  static_cast<void>(argv);

  auto bubble =
      bubble::Bubble<bubble::Context, bubble::Resources, bubble::Renderer>{};

  bubble::Timer init_timer;
  init_timer.start();
  auto config = bubble::InitializeConfig{};
  config.height = 600;
  config.width = config.height * (4.0f / 3.0f);
  // const auto ok = bubble.initialize(config);
  bubble.initialize(config);

  auto duration = init_timer.stop();
  if (duration.has_value()) {
    std::println("Initialized '{}' {}x{} in {}s", config.name, config.width,
                 config.height, duration.value().count());
  }

  bubble::Renderer::set_clearcolor(bubble::color::pleasant_blue);

  auto camera =
      bubble::Camera(glm::vec3(0.0f, 0.0f, -3.0f), glm::vec3(0.0f, 0.0f, 1.0f));

  const auto cube_location = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f));
  const auto cube_color = bubble::color::ugly_pink;

  auto button_focus = bubble::input::Button(SDLK_f);
  auto axis_x = bubble::input::Axis(SDLK_d, SDLK_a);
  auto axis_z = bubble::input::Axis(SDLK_s, SDLK_w);

  auto mouse = bubble::input::Mouse();

  auto should_run = true;
  while (should_run) {
    bubble.begin_frame();
    should_run = !bubble::Context::requested_close();
    const auto deltatime = bubble::Renderer::deltatime().value_or(0.16);

    mouse.update();

    if (mouse.right_pressed()) {
      mouse.lock();
    }
    if (mouse.right_released()) {
      mouse.unlock();
    }
    if (mouse.locked()) {
      const auto mouse_delta = mouse.move_delta();
      const auto rotate_speed = 0.045f;
      const auto rotation = glm::vec2(static_cast<float>(mouse_delta.x),
                                      static_cast<float>(mouse_delta.y));
      camera.rotate(rotation * rotate_speed * static_cast<float>(deltatime));
    }

    button_focus.update();
    if (button_focus.pressed()) {
      camera.look_at(glm::vec3(0.0f));
    }

    axis_x.update();
    axis_z.update();
    const auto move_direction =
        glm::normalize(glm::vec3(axis_x.value(), 0.0f, axis_z.value()));
    if (glm::length(move_direction) > 0.1f) {
      const auto move_speed = 2.0f;
      camera.translate_local(move_direction * static_cast<float>(deltatime) *
                             move_speed);
    }

    bubble::Renderer::set_camera(camera);

    bubble::Renderer::draw_flatcolor(
        cube_location, bubble::Resources::default_mesh(), cube_color);

    bubble::Renderer::draw_textured(
        glm::translate(cube_location, glm::vec3(2.0f, 0.0f, 0.0f)),
        bubble::Resources::default_mesh(),
        bubble::Resources::default_texture());

    bubble.end_frame();
  }

  bubble.destroy();
  return 0;
}
