#include <iostream>
#include <stdexcept>
#include <vulkan/vulkan.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include "Camera.hpp"
#include "EventHandler.hpp"
#include "ve_log.hpp"
#include "vk/VulkanCommandContext.hpp"
#include "vk/VulkanMainContext.hpp"
#include "vk/VulkanRenderContext.hpp"

struct RenderingInfo {
    RenderingInfo(uint32_t width, uint32_t height) : width(width), height(height)
    {}
    uint32_t width;
    uint32_t height;
};

class MainContext
{
public:
    MainContext(const RenderingInfo& ri) : vmc(ri.width, ri.height), vcc(vmc), vrc(vmc, vcc), camera(45.0f, ri.width, ri.height)
    {}

    ~MainContext()
    {
        vcc.self_destruct();
        vrc.self_destruct();
        vmc.self_destruct();
        VE_LOG_CONSOLE(VE_INFO, VE_C_PINK << "Destroyed MainContext" << std::endl);
    }

    void run()
    {
        auto t1 = std::chrono::high_resolution_clock::now();
        auto t2 = std::chrono::high_resolution_clock::now();
        double duration = 0.0;
        bool quit = false;
        SDL_Event e;
        while (!quit)
        {
            dispatch_pressed_keys();
            try
            {
                vrc.update_uniform_data(duration / 1000.0f, camera.getVP());
                vrc.draw_frame();
            }
            catch (const vk::OutOfDateKHRError e)
            {
                vrc.recreate_swapchain();
            }
            while (SDL_PollEvent(&e))
            {
                quit = e.window.event == SDL_WINDOWEVENT_CLOSE;
                eh.dispatch_event(e);
            }
            t2 = std::chrono::high_resolution_clock::now();
            duration = std::chrono::duration<double, std::milli>(t2 - t1).count();
            vmc.window->set_title(ve::to_string(duration, 4) + " ms; FPS: " + ve::to_string(1000.0 / duration));
            t1 = t2;
        }
    }

private:
    ve::VulkanMainContext vmc;
    ve::VulkanCommandContext vcc;
    ve::VulkanRenderContext vrc;
    Camera camera;
    EventHandler eh;
    float move_amount = 0.2f;

    void dispatch_pressed_keys()
    {
        if (eh.pressed_keys.contains(Key::W)) camera.moveFront(move_amount);
        if (eh.pressed_keys.contains(Key::A)) camera.moveRight(-move_amount);
        if (eh.pressed_keys.contains(Key::S)) camera.moveFront(-move_amount);
        if (eh.pressed_keys.contains(Key::D)) camera.moveRight(move_amount);
        if (eh.pressed_keys.contains(Key::Q)) camera.moveDown(move_amount);
        if (eh.pressed_keys.contains(Key::E)) camera.moveDown(-move_amount);
        
        if (eh.pressed_keys.contains(Key::Plus))
        {
            move_amount += 0.05f;
            eh.pressed_keys.erase(Key::Plus);
        }
        if (eh.pressed_keys.contains(Key::Minus))
        {
            move_amount -= 0.05f;
            eh.pressed_keys.erase(Key::Minus);
        }
        if (eh.pressed_keys.contains(Key::MouseRight))
        {
            if (!SDL_GetRelativeMouseMode()) SDL_SetRelativeMouseMode(SDL_TRUE);
            camera.onMouseMove(eh.mouse_motion.x, eh.mouse_motion.y);
            eh.mouse_motion = glm::vec2(0.0f);
        }
        if (eh.released_keys.contains(Key::MouseRight))
        {
            if (SDL_GetRelativeMouseMode()) SDL_SetRelativeMouseMode(SDL_FALSE);
            eh.released_keys.erase(Key::MouseRight);
        }
    }
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
