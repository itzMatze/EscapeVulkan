#include "Window.hpp"

#include "ve_log.hpp"

Window::Window(const uint32_t width, const uint32_t height)
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    window = SDL_CreateWindow("Vulkan Engine", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_VULKAN | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);
}

void Window::self_destruct()
{
    SDL_DestroyWindow(window);
    SDL_Quit();
}

SDL_Window* Window::get() const
{
    return window;
}

std::vector<const char*> Window::get_required_extensions() const
{
    std::vector<const char*> extensions;
    uint32_t extension_count;
    VE_ASSERT(SDL_Vulkan_GetInstanceExtensions(window, &extension_count, nullptr), "Failed to load extension count for window!");
    extensions.resize(extension_count);
    VE_ASSERT(SDL_Vulkan_GetInstanceExtensions(window, &extension_count, extensions.data()), "Failed to load required extensions for window!");
    return extensions;
}

void Window::set_title(const std::string& title)
{
    SDL_SetWindowTitle(window, title.c_str());
}

vk::SurfaceKHR Window::create_surface(const vk::Instance& instance)
{
    vk::SurfaceKHR surface;
    VE_ASSERT(SDL_Vulkan_CreateSurface(window, instance, reinterpret_cast<VkSurfaceKHR*>(&surface)), "Failed to create surface!");
    return surface;
}
