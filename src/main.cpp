#include <iostream>
#include <stdexcept>
#include <vulkan/vulkan.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include "Window.hpp"
#include "ve_log.hpp"
#include "vk/Buffer.hpp"
#include "vk/CommandPool.hpp"
#include "vk/Instance.hpp"
#include "vk/LogicalDevice.hpp"
#include "vk/PhysicalDevice.hpp"
#include "vk/Pipeline.hpp"
#include "vk/Swapchain.hpp"
#include "vk/Synchronization.hpp"
#include "vk/VulkanCommandContext.hpp"
#include "vk/VulkanMainContext.hpp"
#include "vk/VulkanRenderContext.hpp"
#include "vk/common.hpp"

struct RenderingInfo {
    RenderingInfo(uint32_t width, uint32_t height) : width(width), height(height)
    {}
    uint32_t width;
    uint32_t height;
};

class MainContext
{
public:
    MainContext(const RenderingInfo& ri) : vmc(ri.width, ri.height), vrc(vmc), vcc(vmc, vrc)
    {}

    ~MainContext()
    {
       vcc.self_destruct();
       vrc.self_destruct();
       vmc.self_destruct(); 
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
            try
            {
                vcc.draw_frame();
            }
            catch (const vk::OutOfDateKHRError e)
            {
                vcc.recreate_swapchain();
            }
            while (SDL_PollEvent(&e))
            {
                quit = e.window.event == SDL_WINDOWEVENT_CLOSE;
            }
            t2 = std::chrono::high_resolution_clock::now();
            duration = std::chrono::duration<double, std::milli>(t2 - t1).count();
            vmc.window.set_title(ve::to_string(duration, 4) + " ms; FPS: " + ve::to_string(1000.0 / duration));
            t1 = t2;
        }
    }

private:
    ve::VulkanMainContext vmc;
    ve::VulkanRenderContext vrc;
    ve::VulkanCommandContext vcc;
};

int main(int argc, char** argv)
{
    VE_LOG_CONSOLE(VE_INFO, "Starting\n");
    auto t1 = std::chrono::high_resolution_clock::now();
    RenderingInfo ri(1000, 800);
    MainContext mc(ri);
    auto t2 = std::chrono::high_resolution_clock::now();
    VE_LOG_CONSOLE(VE_INFO, VE_C_BLUE << "Setup took: " << (std::chrono::duration<double, std::milli>(t2 - t1).count()) << "ms" << std::endl);
    mc.run();
    return 0;
}
