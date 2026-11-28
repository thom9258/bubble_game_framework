#include "bubble/initialize_config.hpp"
#include <bubble/animation.hpp>
#include <bubble/bitmap.hpp>
#include <bubble/bubble.hpp>
#include <bubble/context.hpp>
#include <bubble/input.hpp>
#include <bubble/model.hpp>
#include <bubble/renderer.hpp>
#include <bubble/resources.hpp>
#include <bubble/timer.hpp>

#include <bubble/internal/glm.hpp>
#include <bubble/internal/imgui.hpp>
#include <bubble/internal/sdl.hpp>

#include <print>

int main(int argc, char **argv) {
  static_cast<void>(argc);
  static_cast<void>(argv);

  auto bubble = bubble::Bubble<BUBBLE_REQUIRED_SYSTEMS>{};

  bubble::Timer init_timer;
  init_timer.start();
  auto config = bubble::InitializeConfig{};
  config.height = 600;
  config.width = config.height * (4.0f / 3.0f);
  config.options.graphics_validation = true;
  config.options.presentation_mode = bubble::PresentationMode::VSync;

  if (!bubble.initialize(config)) {
    return 1;
  }

  auto duration = init_timer.stop();
  if (duration.has_value()) {
    std::println("Initialized '{}' {}x{} in {}s", config.name, config.width,
                 config.height, duration.value().count());
  }

  bubble::Renderer::set_clearcolor(bubble::color::pleasant_blue);

  auto fox = bubble::load_model(
      "/home/th/Assets/glTF-Sample-Models/2.0/Fox/glTF-Binary/Fox.glb");
  if (!fox) {
    std::println("Fox load error: '{}'", fox.error());
    return 1;
  }

  for (std::size_t i = 0; i < fox->scene.animations.size(); ++i) {
    const auto &clip = fox->scene.animations[i];
    const auto &anim = clip.animation;
    std::println("animation {}: '{}' {:.2f}s, {} bones", i, clip.name,
                 anim.total_ticks().get() / anim.ticks_per_second().get(),
                 anim.m_bones.size());
  }

  const auto fox_animations = std::vector{"Survey", "Walk", "Run"};
  std::size_t current_fox_animation = 0;

  auto fox_animator = bubble::Animator(*fox);
  if (!fox_animator.play(fox_animations[current_fox_animation])) {
    std::println("no clip named 'Walk'");
  }

  auto camera =
      bubble::Camera(glm::vec3(0.0f, 0.0f, -3.0f), glm::vec3(0.0f, 0.0f, 1.0f));

  const auto cube_color = bubble::color::ugly_pink;

  auto directional_light = bubble::DirectionalLight{};
  directional_light.direction = {-0.4f, -1.0f, -0.3f};
  directional_light.color = {255, 240, 210, 255};
  directional_light.intensity = 0.8f;

  auto hemisphere_light = bubble::HemisphereLight{};
  hemisphere_light.color = {70, 60, 50, 255};
  hemisphere_light.sky = {150, 170, 200, 255};
  hemisphere_light.intensity = 0.6f;

  auto should_run = true;
  while (should_run) {
    bubble.begin_frame();
    should_run = !bubble::Context::requested_close();
    const auto deltatime = bubble::Renderer::deltatime().value_or(0.16);

    bubble::Input::mouse_set_locked(
        bubble::Input::down(bubble::MouseButton::Right));

    if (bubble::Input::mouse_locked()) {
      const auto mouse_delta = bubble::Input::mouse_delta();
      const auto rotation = glm::vec2(static_cast<float>(mouse_delta.dx),
                                      static_cast<float>(mouse_delta.dy));
      const auto mouse_rotate_speed = 0.0013f;
      camera.rotate(rotation * mouse_rotate_speed);
    }

    if (bubble::Input::gamepad_connected()) {
      const auto rotation_x = bubble::Input::axis(bubble::GamepadAxis::RightX);
      const auto rotation_y = bubble::Input::axis(bubble::GamepadAxis::RightY);
      const auto gamepad_rotate_speed = 0.02f;
      camera.rotate(glm::vec2(rotation_x, rotation_y) * gamepad_rotate_speed);
    }

    if (bubble::Input::pressed(bubble::Key::F) ||
        bubble::Input::pressed(bubble::GamepadButton::South)) {
      camera.look_at(glm::vec3(0.0f));
    }

    if (bubble::Input::pressed(bubble::Key::N) ||
        bubble::Input::pressed(bubble::GamepadButton::East)) {
      current_fox_animation++;
      if (current_fox_animation >= fox_animations.size()) {
        current_fox_animation = 0;
      }

      if (!fox_animator.play(fox_animations[current_fox_animation])) {
        std::println("no clip named 'Walk'");
      }
    }

    const auto axis_x = bubble::Input::axis(bubble::Key::D, bubble::Key::A) -
                        bubble::Input::axis(bubble::GamepadAxis::LeftX);
    const auto axis_z = bubble::Input::axis(bubble::Key::S, bubble::Key::W) -
                        bubble::Input::axis(bubble::GamepadAxis::LeftY);
    const auto move_direction = glm::normalize(glm::vec3(axis_x, 0.0f, axis_z));
    if (glm::length(move_direction) > 0.1f) {
      const auto move_speed = 2.0f;
      camera.translate_local(move_direction * static_cast<float>(deltatime) *
                             move_speed);
    }

    bubble::Renderer::set_camera(camera);
    bubble::Renderer::add_light(directional_light);
    bubble::Renderer::add_light(hemisphere_light);

    bubble::Renderer::draw_flatcolor(
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f)),
        bubble::Resources::default_mesh(), cube_color);

    bubble::Renderer::draw_textured(
        glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
        bubble::Resources::default_mesh(),
        bubble::Resources::default_texture());

    const auto fox_transform = glm::scale(
        glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, 0.0f, 0.0f)),
        glm::vec3(0.02f));

    fox_animator.update(static_cast<float>(deltatime));
    bubble::draw_model(*fox, fox_transform, fox_animator);

    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::Begin("Debug", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                     ImGuiWindowFlags_AlwaysAutoResize |
                     ImGuiWindowFlags_NoSavedSettings);

    if (ImGui::Button("Exit")) {
      should_run = false;
    }

    ImGui::SameLine();
    const float fps = ImGui::GetIO().Framerate;
    ImGui::Text("%.0f FPS (%.2f ms)", fps, 1000.0f / fps);
    ImGui::End();
    bubble.end_frame();
  }

  if (fox.has_value()) {
    bubble::unload_model(*fox);
  }

  bubble.destroy();
  return 0;
}
