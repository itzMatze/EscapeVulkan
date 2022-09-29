#pragma once

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

class Window
{
public:
    Window(const uint32_t width, const uint32_t height)
    {
        SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
        window = SDL_CreateWindow("Vulkan Engine", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_VULKAN | SDL_WINDOW_ALLOW_HIGHDPI);
    }

    ~Window()
    {
        SDL_DestroyWindow(window);
        SDL_Quit();
    }

    bool get_required_extensions(std::vector<const char*>& extensions)
    {
        uint32_t extension_count;
        bool success = true;
        SDL_Vulkan_GetInstanceExtensions(window, &extension_count, nullptr);
        extensions.resize(extension_count);
        success &= SDL_Vulkan_GetInstanceExtensions(window, &extension_count, extensions.data());
        return success;
    }

    SDL_Window* window;
private:
};