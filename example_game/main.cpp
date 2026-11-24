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
  config.options.enable_graphics_validation = false;
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

  auto loaded_fox = bubble::load_gltf_scene(
      "/home/th/Assets/glTF-Sample-Models/2.0/Fox/glTF-Binary/Fox.glb");

  if (!loaded_fox.has_value()) {
    std::println("Fox load error: '{}'", loaded_fox.error());
    return 1;
  }

  auto fox = create_model_tree(loaded_fox.value());
  if (!fox.has_value()) {
    std::println("Fox create error: '{}'", fox.error());
    return 1;
  }

  // --- Fox: animations + skinned mesh ----------------------------------------
  auto &fox_scene = loaded_fox.value();
  for (std::size_t i = 0; i < fox_scene.animations.size(); ++i) {
    auto &a = fox_scene.animations[i];
    std::println("animation {}: '{}' {:.2f}s, {} bones", i, a.name,
                 a.animation.total_ticks().get(), a.animation.m_bones.size());
  }

  // The Fox is a single skinned mesh: interleave Vertex + BoneVertex
  // into AnimatedVertex for the animation pipeline.
  const auto &fox_mesh_data = fox_scene.meshes.at(0);
  auto fox_mesh = bubble::Resources::create_animated_mesh(
      fox_mesh_data.vertices, fox_mesh_data.bone_vertices,
      fox_mesh_data.indices);
  if (!fox_mesh.has_value()) {
    std::println("Fox animated mesh create error");
    return 1;
  }

  // Reuse the texture the model tree already uploaded:
  // material -> bitmap index -> created_textures (same indices as bitmaps).
  auto fox_texture = bubble::Resources::default_texture();
  if (fox_mesh_data.material) {
    if (const auto bitmap =
            fox_scene.materials[*fox_mesh_data.material].diffuse) {
      fox_texture = fox->unique_textures.at(*bitmap);
    }
  }

  std::size_t current_clip = fox_scene.animations.size() > 1 ? 1 : 0; // Walk
  double animation_time = 0.0;

  auto camera =
      bubble::Camera(glm::vec3(0.0f, 0.0f, -3.0f), glm::vec3(0.0f, 0.0f, 1.0f));

  const auto cube_color = bubble::color::ugly_pink;

  // auto button_focus = bubble::Button(SDLK_f);
  // auto axis_x = bubble::ButtonAxis(SDLK_d, SDLK_a);
  // auto axis_z = bubble::ButtonAxis(SDLK_s, SDLK_w);
  //
  // auto mouse = bubble::Mouse();

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
      const auto rotate_speed = 0.0013f;
      const auto rotation = glm::vec2(static_cast<float>(mouse_delta.dx),
                                      static_cast<float>(mouse_delta.dy));
      camera.rotate(rotation * rotate_speed);
    }

    if (bubble::Input::pressed(bubble::Key::F)) {
      camera.look_at(glm::vec3(0.0f));
    }

    //   axis_x.update();
    //   axis_z.update();
    const auto axis_x = bubble::Input::axis(bubble::Key::D, bubble::Key::A);
    const auto axis_z = bubble::Input::axis(bubble::Key::S, bubble::Key::W);
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

    if (!fox_scene.animations.empty()) {
      animation_time += deltatime;

      auto &clip = fox_scene.animations[current_clip].animation;
      auto pose =
          clip.animate(animation_time); // loops: fmod over the clip length
      auto bones = bubble::animation::calculate_final_animation_state(
          clip.bone_infos(), pose);

      if (bones.has_value()) {
        bubble::Renderer::draw_animated(fox_transform, *fox_mesh, fox_texture,
                                        bones->matrices());
      }
    }

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

  bubble.destroy();
  return 0;
}
