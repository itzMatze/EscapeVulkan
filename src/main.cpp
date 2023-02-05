#include <iostream>
#include <stdexcept>
#include <thread>
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

class MainContext
{
public:
    MainContext() : extent(1000, 800), vmc(extent.width, extent.height), vcc(vmc), vrc(vmc, vcc), camera(45.0f, extent.width, extent.height)
    {
        extent = vrc.swapchain.get_extent();
        camera.updateScreenSize(extent.width, extent.height);
    }

    ~MainContext()
    {
        vcc.self_destruct();
        vrc.self_destruct();
        vmc.self_destruct();
        spdlog::info("Destroyed MainContext");
    }

    void run()
    {
        constexpr double min_frametime = 5.0;
        auto t1 = std::chrono::high_resolution_clock::now();
        auto t2 = std::chrono::high_resolution_clock::now();
        // keep time measurement and frametime separate to be able to use a frame limiter
        double frametime = 0.0;
        bool quit = false;
        SDL_Event e;
        while (!quit)
        {
            move_amount = di.time_diff * move_speed;
            dispatch_pressed_keys();
            try
            {
                std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(min_frametime - frametime));
                di.time_diff /= 1000.0f;
                vrc.draw_frame(camera, di);
            }
            catch (const vk::OutOfDateKHRError e)
            {
                extent = vrc.recreate_swapchain();
                camera.updateScreenSize(extent.width, extent.height);
            }
            while (SDL_PollEvent(&e))
            {
                quit = e.window.event == SDL_WINDOWEVENT_CLOSE;
                eh.dispatch_event(e);
            }
            t2 = std::chrono::high_resolution_clock::now();
            di.time_diff = std::chrono::duration<double, std::milli>(t2 - t1).count();
            // calculate actual frametime by subtracting the waiting time
            frametime = di.time_diff - std::max(0.0, min_frametime - frametime);
            vmc.window->set_title(ve::to_string(di.time_diff, 4) + " ms; FPS: " + ve::to_string(1000.0 / di.time_diff) + " (" + ve::to_string(frametime, 4) + " ms; FPS: " + ve::to_string(1000.0 / frametime) + ")");
            t1 = t2;
        }
    }

private:
    vk::Extent2D extent;
    ve::VulkanMainContext vmc;
    ve::VulkanCommandContext vcc;
    ve::VulkanRenderContext vrc;
    Camera camera;
    EventHandler eh;
    float move_amount;
    float move_speed = 0.02f;
    ve::DrawInfo di;

    void dispatch_pressed_keys()
    {
        if (eh.pressed_keys.contains(Key::W)) camera.moveFront(move_amount);
        if (eh.pressed_keys.contains(Key::A)) camera.moveRight(-move_amount);
        if (eh.pressed_keys.contains(Key::S)) camera.moveFront(-move_amount);
        if (eh.pressed_keys.contains(Key::D)) camera.moveRight(move_amount);
        if (eh.pressed_keys.contains(Key::Q)) camera.moveDown(move_amount);
        if (eh.pressed_keys.contains(Key::E)) camera.moveDown(-move_amount);

        // reset state of keys that are used to execute a one time action
        if (eh.pressed_keys.contains(Key::Plus))
        {
            move_speed += 0.01f;
            eh.pressed_keys.erase(Key::Plus);
        }
        if (eh.pressed_keys.contains(Key::Minus))
        {
            move_speed -= 0.01f;
            eh.pressed_keys.erase(Key::Minus);
        }
        if (eh.pressed_keys.contains(Key::MouseLeft))
        {
            if (!SDL_GetRelativeMouseMode()) SDL_SetRelativeMouseMode(SDL_TRUE);
            camera.onMouseMove(eh.mouse_motion.x * 1.5f, eh.mouse_motion.y * 1.5f);
            eh.mouse_motion = glm::vec2(0.0f);
        }
        if (eh.released_keys.contains(Key::MouseLeft))
        {
            if (SDL_GetRelativeMouseMode()) SDL_SetRelativeMouseMode(SDL_FALSE);
            eh.released_keys.erase(Key::MouseLeft);
        }
    }
};

int main(int argc, char** argv)
{
    std::vector<spdlog::sink_ptr> sinks;
    sinks.push_back(std::make_shared<spdlog::sinks::stdout_sink_st>());
    sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_st>("ve.log", true));
    auto combined_logger = std::make_shared<spdlog::logger>("default_logger", begin(sinks), end(sinks));
    spdlog::set_default_logger(combined_logger);
    spdlog::set_level(spdlog::level::debug);
    spdlog::info("Starting");
    auto t1 = std::chrono::high_resolution_clock::now();
    MainContext mc;
    auto t2 = std::chrono::high_resolution_clock::now();
    spdlog::info("Setup took: {} ms", (std::chrono::duration<double, std::milli>(t2 - t1).count()));
    mc.run();
    return 0;
}
