#include <iostream>
#include <stdexcept>
#include <vulkan/vulkan.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include "ve_log.h"
#include "Window.h"
#include "vk/Instance.h"
#include "vk/PhysicalDevice.h"
#include "vk/LogicalDevice.h"

struct RenderingInfo
{
    RenderingInfo(uint32_t width, uint32_t height) : width(width), height(height)
    {}
    uint32_t width;
    uint32_t height;
};

class MainContext
{
public:
    MainContext(const RenderingInfo& ri) : window(ri.width, ri.height), instance(&(this->window), required_extensions, optional_extensions, validation_layers), physical_device(this->instance), logical_device(physical_device)
    {
        VE_LOG_CONSOLE("Creating MainContext");
    }

private:
    const std::vector<const char*> required_extensions;
    const std::vector<const char*> optional_extensions{ "Test", "Test1" };
    const std::vector<const char*> validation_layers{ "VK_LAYER_KHRONOS_validation" };
    Window window;
    ve::Instance instance;
    ve::PhysicalDevice physical_device;
    ve::LogicalDevice logical_device;
};

int main(int argc, char** argv)
{
    RenderingInfo ri(1000, 800);
    MainContext mc(ri);

    bool quit = false;
    while (!quit)
    {
        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            quit = e.window.event == SDL_WINDOWEVENT_CLOSE;
        }
    }
    return 0;
}
