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
    renderer::destroy();
    context::destroy();
    resources::destroy();
}

auto next_frame() -> void { context::next_frame(); }

auto requested_close() -> bool { return context::requested_close(); }

} // namespace bubble
