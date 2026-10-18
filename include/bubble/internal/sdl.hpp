#pragma once

#include <vulkan/vulkan.hpp>
#include <SDL2/SDL.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_mouse.h>
#include <SDL2/SDL_vulkan.h>

#include <vector>

namespace bubble
{

auto get_sdl_events() -> std::vector<SDL_Event>;

}
