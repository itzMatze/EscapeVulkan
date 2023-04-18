#include <iostream>
#include <filesystem>
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
#include "vk/VulkanMainContext.hpp"
#include "vk/VulkanCommandContext.hpp"
#include "vk/VulkanStorageContext.hpp"
#include "vk/VulkanRenderContext.hpp"

class MainContext
{
public:
    MainContext() : extent(1000, 800), vmc(extent.width, extent.height), vcc(vmc), vsc(vmc, vcc), vrc(vmc, vcc, vsc), camera(45.0f, extent.width, extent.height)
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
        std::vector<std::string> scene_names;
        for (const auto& entry : std::filesystem::directory_iterator("../assets/scenes/"))
        {
            scene_names.push_back(entry.path().filename());
        }
        di.current_scene = 0;
        for (const auto& name : scene_names) di.scene_names.push_back(&name.front());
        vrc.load_scene(di.scene_names[di.current_scene]);
        constexpr float min_frametime = 5.0f;
        auto t1 = std::chrono::high_resolution_clock::now();
        auto t2 = std::chrono::high_resolution_clock::now();
        // keep time measurement and frametime separate to be able to use a frame limiter
        bool quit = false;
        SDL_Event e;
        while (!quit)
        {
            move_amount = di.time_diff * move_speed;
            dispatch_pressed_keys();
            di.vp = camera.getVP();
            try
            {
                std::this_thread::sleep_for(std::chrono::duration<float, std::milli>(min_frametime - di.frametime));
                di.time_diff /= 1000.0f;
                vrc.draw_frame(di);
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
            di.time_diff = std::chrono::duration<float, std::milli>(t2 - t1).count();
            // calculate actual frametime by subtracting the waiting time
            di.frametime = di.time_diff - std::max(0.0f, min_frametime - di.frametime);
            t1 = t2;
            if (di.load_scene)
            {
                di.load_scene = false;
                vrc.load_scene(di.scene_names[di.current_scene]);
                t1 = std::chrono::high_resolution_clock::now();
            }
        }
    }

private:
    vk::Extent2D extent;
    ve::VulkanMainContext vmc;
    ve::VulkanCommandContext vcc;
    ve::VulkanStorageContext vsc;
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
        float panning_speed = eh.pressed_keys.contains(Key::Shift) ? 0.2f : 2.0f;
        if (eh.pressed_keys.contains(Key::Left)) camera.onMouseMove(-panning_speed, 0.0f);
        if (eh.pressed_keys.contains(Key::Right)) camera.onMouseMove(panning_speed, 0.0f);
        if (eh.pressed_keys.contains(Key::Up)) camera.onMouseMove(0.0f, -panning_speed);
        if (eh.pressed_keys.contains(Key::Down)) camera.onMouseMove(0.0f, panning_speed);

        // reset state of keys that are used to execute a one time action
        if (eh.pressed_keys.contains(Key::Plus))
        {
            move_speed *= 2.0f;
            eh.pressed_keys.erase(Key::Plus);
        }
        if (eh.pressed_keys.contains(Key::Minus))
        {
            move_speed /= 2.0f;
            eh.pressed_keys.erase(Key::Minus);
        }
        if (eh.pressed_keys.contains(Key::G))
        {
            di.show_ui = !di.show_ui;
            eh.pressed_keys.erase(Key::G);
        }
        if (eh.pressed_keys.contains(Key::M))
        {
            di.mesh_view = !di.mesh_view;
            eh.pressed_keys.erase(Key::M);
        }
        if (eh.pressed_keys.contains(Key::MouseLeft))
        {
            if (!SDL_GetRelativeMouseMode()) SDL_SetRelativeMouseMode(SDL_TRUE);
            camera.onMouseMove(eh.mouse_motion.x * 1.5f, eh.mouse_motion.y * 1.5f);
            eh.mouse_motion = glm::vec2(0.0f);
        }
        if (eh.released_keys.contains(Key::MouseLeft))
        {
            SDL_SetRelativeMouseMode(SDL_FALSE);
            SDL_WarpMouseInWindow(vmc.window->get(), extent.width / 2.0f, extent.height / 2.0f);
            eh.released_keys.erase(Key::MouseLeft);
        }
    }
};

int main(int argc, char** argv)
{
    std::vector<spdlog::sink_ptr> sinks;
    sinks.push_back(std::make_shared<spdlog::sinks::stdout_sink_st>());
    sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_st>("ve.log", true));
    auto combined_logger = std::make_shared<spdlog::logger>("default_logger", sinks.begin(), sinks.end());
    spdlog::set_default_logger(combined_logger);
    spdlog::set_level(spdlog::level::debug);
    spdlog::set_pattern("[%Y-%m-%d %T.%e] [%L] %v");
    spdlog::info("Starting");
    auto t1 = std::chrono::high_resolution_clock::now();
    MainContext mc;
    auto t2 = std::chrono::high_resolution_clock::now();
    spdlog::info("Setup took: {} ms", (std::chrono::duration<double, std::milli>(t2 - t1).count()));
    mc.run();
    return 0;
}
