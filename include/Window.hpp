#pragma once

#include <vector>
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include "vk/common.hpp"

class Window
{
public:
    Window(const uint32_t width, const uint32_t height);
    void self_destruct();
    SDL_Window* get() const;
    std::vector<const char*> get_required_extensions() const;
    void set_title(const std::string& title);
    vk::SurfaceKHR create_surface(const vk::Instance& instance);

private:
    SDL_Window* window;
};