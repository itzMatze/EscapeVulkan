#include <iostream>
#include <stdexcept>
#include <vulkan/vulkan.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include "Window.hpp"
#include "ve_log.hpp"
#include "vk/Instance.hpp"
#include "vk/LogicalDevice.hpp"
#include "vk/PhysicalDevice.hpp"

struct RenderingInfo {
    RenderingInfo(uint32_t width, uint32_t height) : width(width), height(height)
    {}
    uint32_t width;
    uint32_t height;
};

class MainContext
{
public:
    MainContext(const RenderingInfo& ri) : window(ri.width, ri.height), instance(this->window, required_extensions, optional_extensions, validation_layers), physical_device(this->instance, required_device_extensions, optional_device_extensions), logical_device(physical_device)
    {
        logical_device.create_swapchain(instance.get_surface(), window.get());
        VE_LOG_CONSOLE(PINK << "Created MainContext");
    }

    ~MainContext()
    {

    }

private:
    const std::vector<const char*> required_extensions{VK_KHR_SURFACE_EXTENSION_NAME};
    const std::vector<const char*> optional_extensions{};
    const std::vector<const char*> validation_layers{"VK_LAYER_KHRONOS_validation"};

    const std::vector<const char*> required_device_extensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    const std::vector<const char*> optional_device_extensions{VK_KHR_RAY_QUERY_EXTENSION_NAME, VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME};
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
