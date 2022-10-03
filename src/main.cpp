#include <iostream>
#include <stdexcept>
#include <vulkan/vulkan.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include "Window.hpp"
#include "ve_log.hpp"
#include "vk/CommandPool.hpp"
#include "vk/Instance.hpp"
#include "vk/LogicalDevice.hpp"
#include "vk/PhysicalDevice.hpp"
#include "vk/Pipeline.hpp"
#include "vk/Swapchain.hpp"
#include "vk/Synchronization.hpp"

struct RenderingInfo {
    RenderingInfo(uint32_t width, uint32_t height) : width(width), height(height)
    {}
    uint32_t width;
    uint32_t height;
};

class MainContext
{
public:
    MainContext(const RenderingInfo& ri) : window(ri.width, ri.height), instance(window, required_extensions, optional_extensions, validation_layers), physical_device(instance, required_device_extensions, optional_device_extensions), logical_device(physical_device), swapchain(physical_device, logical_device.get(), instance.get_surface(), window.get()), pipeline(logical_device.get(), swapchain.get_render_pass()), command_pool(logical_device.get(), physical_device.get_queue_families().graphics, frames_in_flight), sync(logical_device.get())
    {
        for (uint32_t i = 0; i < frames_in_flight; ++i)
        {
            sync_indices.push_back(sync.add_semaphore());
            sync_indices.push_back(sync.add_semaphore());
            sync_indices.push_back(sync.add_fence());
        }
        VE_LOG_CONSOLE(VE_INFO, VE_C_PINK << "Created MainContext\n");
    }

    ~MainContext()
    {
        VE_LOG_CONSOLE(VE_INFO, VE_C_GREEN << "Destruction MainContext\n");
    }

    void run()
    {
        auto t1 = std::chrono::high_resolution_clock::now();
        auto t2 = std::chrono::high_resolution_clock::now();
        double duration;
        bool quit = false;
        SDL_Event e;
        while (!quit)
        {
            draw_frame();
            while (SDL_PollEvent(&e))
            {
                quit = e.window.event == SDL_WINDOWEVENT_CLOSE;
            }
            t2 = std::chrono::high_resolution_clock::now();
            duration = std::chrono::duration<double, std::milli>(t2 - t1).count();
            window.set_title(ve::to_string(duration, 4) + " ms; FPS: " + ve::to_string(1000.0 / duration));
            t1 = t2;
        }
        wait_to_finish();
    }

    void draw_frame()
    {
        glm::vec3 draw_sync_indices(sync_indices[0 + 3 * current_frame], sync_indices[1 + 3 * current_frame], sync_indices[2 + 3 * current_frame]);
        vk::ResultValue<uint32_t> image_idx = logical_device.get().acquireNextImageKHR(swapchain.get(), uint64_t(-1), sync.get_semaphore(draw_sync_indices.x));
        VE_CHECK(image_idx.result, "Failed to acquire next image!");
        command_pool.reset_command_buffer(current_frame);
        command_pool.record_command_buffer(current_frame, swapchain, pipeline.get(), image_idx.value);
        logical_device.submit_graphics(sync, draw_sync_indices, vk::PipelineStageFlagBits::eColorAttachmentOutput, command_pool.get_buffer(current_frame), swapchain.get(), image_idx.value);
    }

    void wait_to_finish()
    {
        logical_device.get().waitIdle();
    }

private:
    const std::vector<const char*> required_extensions{VK_KHR_SURFACE_EXTENSION_NAME};
    const std::vector<const char*> optional_extensions{};
    const std::vector<const char*> validation_layers{"VK_LAYER_KHRONOS_validation"};

    const std::vector<const char*> required_device_extensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    const std::vector<const char*> optional_device_extensions{VK_KHR_RAY_QUERY_EXTENSION_NAME, VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME};

    const uint32_t frames_in_flight = 2;
    uint32_t current_frame = 0;

    Window window;
    ve::Instance instance;
    ve::PhysicalDevice physical_device;
    ve::LogicalDevice logical_device;
    ve::Swapchain swapchain;
    ve::Pipeline pipeline;
    ve::CommandPool command_pool;
    ve::Synchronization sync;
    std::vector<uint32_t> sync_indices;
};

int main(int argc, char** argv)
{
    auto t1 = std::chrono::high_resolution_clock::now();
    RenderingInfo ri(1000, 800);
    MainContext mc(ri);
    auto t2 = std::chrono::high_resolution_clock::now();
    VE_LOG_CONSOLE(VE_INFO, VE_C_BLUE << "Setup took: " << (std::chrono::duration<double, std::milli>(t2 - t1).count()) << "ms" << std::endl);
    mc.run();
    return 0;
}
