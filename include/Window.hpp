#pragma once

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include "ve_log.hpp"

class Window
{
public:
    Window(const uint32_t width, const uint32_t height)
    {
        VE_LOG_CONSOLE(VE_INFO, VE_C_PINK << "Creating window\n");
        SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
        window = SDL_CreateWindow("Vulkan Engine", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_VULKAN | SDL_WINDOW_ALLOW_HIGHDPI);
    }

    ~Window()
    {
        SDL_DestroyWindow(window);
        SDL_Quit();
    }

    SDL_Window* get() const
    {
        return window;
    }

    void get_required_extensions(std::vector<const char*>& extensions) const
    {
        uint32_t extension_count;
        VE_ASSERT(SDL_Vulkan_GetInstanceExtensions(window, &extension_count, nullptr), "Failed to load extension count for window!");
        extensions.resize(extension_count);
        VE_ASSERT(SDL_Vulkan_GetInstanceExtensions(window, &extension_count, extensions.data()), "Failed to load required extensions for window!");
    }

private:
    SDL_Window* window;
};