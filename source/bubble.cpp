#include "../include/bubble/bubble.hpp"

#include "context.hpp"
#include "renderer.hpp"
#include "resources.hpp"

namespace bubble {

auto initialize(InitializeInfo &info) -> bool {

  if (!context::initialize(info)) {
    return false;
  }

  if (!renderer::initialize(info)) {
    return false;
  }

  if (!resources::initialize()) {
    return false;
  }

  return true;
}

auto destroy() -> void
{
    context::device().waitIdle();
    resources::destroy();
    renderer::destroy();
    context::destroy();
}

auto begin_frame() -> void { 
    context::begin_frame(); 
    renderer::begin_frame(); 
}

auto set_clearcolor(Color color) -> void { renderer::set_clearcolor(color); }

auto set_camera(Camera camera) -> void { renderer::set_camera(camera); }

auto requested_close() -> bool { return context::requested_close(); }

auto end_frame() -> void
{
    renderer::end_frame();
}

auto draw_flatcolor(glm::mat4 location, MeshRef mesh, Color color) -> void {
    renderer::draw_flatcolor(location, mesh, color);
}

auto deltatime() -> float 
{
    return 0.16f;
}

} // namespace bubble
