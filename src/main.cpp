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
#include "vk/Timer.hpp"
#include "vk/VulkanMainContext.hpp"
#include "vk/VulkanCommandContext.hpp"
#include "Storage.hpp"
#include "WorkContext.hpp"

class MainContext
{
public:
    MainContext() : extent(1000, 800), vmc(extent.width, extent.height), vcc(vmc), wc(vmc, vcc), camera(45.0f, extent.width, extent.height), di{.cam = camera}
    {
        di.devicetimings.resize(ve::DeviceTimer::TIMER_COUNT, 0.0f);
        extent = wc.swapchain.get_extent();
        camera.updateScreenSize(extent.width, extent.height);
    }

    ~MainContext()
    {
        wc.self_destruct();
        vcc.self_destruct();
        vmc.self_destruct();
        spdlog::info("Destroyed MainContext");
    }

    void run()
    {
        std::vector<std::string> scene_names;
        di.current_scene = 0;
        for (const auto& entry : std::filesystem::directory_iterator("../assets/scenes/"))
        {
            if (entry.path().filename() == "escapevulkan.json") di.current_scene = scene_names.size();
            scene_names.push_back(entry.path().filename());
        }
        for (const auto& name : scene_names) di.scene_names.push_back(&name.front());
        wc.load_scene(di.scene_names[di.current_scene]);
        constexpr float min_frametime = 5.0f;
        // keep time measurement and frametime separate to be able to use a frame limiter
        ve::HostTimer timer;
        bool quit = false;
        SDL_Event e;
        while (!quit)
        {
            move_amount = di.time_diff * move_speed;
            dispatch_pressed_keys();
            di.cam.updateVP();
            try
            {
                std::this_thread::sleep_for(std::chrono::duration<float, std::milli>(min_frametime - di.frametime));
                di.time_diff /= 1000.0f;
                di.player_pos = camera.getPosition();
                wc.draw_frame(di);
            }
            catch (const vk::OutOfDateKHRError e)
            {
                extent = wc.recreate_swapchain();
                camera.updateScreenSize(extent.width, extent.height);
            }
            while (SDL_PollEvent(&e))
            {
                quit = e.window.event == SDL_WINDOWEVENT_CLOSE;
                eh.dispatch_event(e);
            }
            di.time_diff = timer.restart();
            di.time += di.time_diff;
            // calculate actual frametime by subtracting the waiting time
            di.frametime = di.time_diff - std::max(0.0f, min_frametime - di.frametime);
            if (di.load_scene)
            {
                di.load_scene = false;
                wc.load_scene(di.scene_names[di.current_scene]);
                timer.restart();
            }
        }
    }

private:
    vk::Extent2D extent;
    ve::VulkanMainContext vmc;
    ve::VulkanCommandContext vcc;
    ve::WorkContext wc;
    Camera camera;
    EventHandler eh;
    float move_amount;
    float move_speed = 0.02f;
    ve::DrawInfo di;
    bool use_controller = false;

    void dispatch_pressed_keys()
    {
        if (eh.is_key_pressed(Key::W)) camera.moveFront(move_amount);
        if (eh.is_key_pressed(Key::A)) camera.moveRight(-move_amount);
        if (eh.is_key_pressed(Key::S)) camera.moveFront(-move_amount);
        if (eh.is_key_pressed(Key::D)) camera.moveRight(move_amount);
        if (eh.is_key_pressed(Key::Q)) camera.moveDown(move_amount);
        if (eh.is_key_pressed(Key::E)) camera.moveDown(-move_amount);
        float panning_speed = eh.is_key_pressed(Key::Shift) ? 0.2f : 2.0f;
        if (eh.is_key_pressed(Key::Left)) camera.onMouseMove(-panning_speed, 0.0f);
        if (eh.is_key_pressed(Key::Right)) camera.onMouseMove(panning_speed, 0.0f);
        if (eh.is_key_pressed(Key::Up)) camera.onMouseMove(0.0f, -panning_speed);
        if (eh.is_key_pressed(Key::Down)) camera.onMouseMove(0.0f, panning_speed);

        // reset state of keys that are used to execute a one time action
        if (eh.is_key_released(Key::Plus))
        {
            move_speed *= 2.0f;
            eh.set_released_key(Key::Plus, false);
        }
        if (eh.is_key_released(Key::Minus))
        {
            move_speed /= 2.0f;
            eh.set_released_key(Key::Minus, false);
        }
        if (eh.is_key_released(Key::G))
        {
            di.show_ui = !di.show_ui;
            eh.set_released_key(Key::G, false);
        }
        if (eh.is_key_released(Key::M))
        {
            di.mesh_view = !di.mesh_view;
            eh.set_released_key(Key::M, false);
        }
        if (eh.is_key_released(Key::N))
        {
            di.normal_view = !di.normal_view;
            eh.set_released_key(Key::N, false);
        }
        if (eh.is_key_released(Key::T))
        {
            di.tex_view = !di.tex_view;
            eh.set_released_key(Key::T, false);
        }
        if (eh.is_key_released(Key::F))
        {
            camera.is_tracking_camera = !camera.is_tracking_camera;
            eh.set_released_key(Key::F, false);
        }
        if (eh.is_key_released(Key::F12))
        {
            di.save_screenshot = true;
            eh.set_released_key(Key::F12, false);
        }
        if (eh.is_key_released(Key::X) && eh.is_controller_available())
        {
            use_controller = !use_controller;
            eh.set_released_key(Key::X, false);
        }
        if (eh.is_key_released(Key::R))
        {
            system("cmake --build . --target Shaders");
            eh.set_released_key(Key::R, false);
            wc.reload_shaders();
        }
        if (use_controller)
        {
            std::pair<glm::vec2, glm::vec2> joystick_pos = eh.get_controller_joystick_pos();
            camera.onMouseMove(joystick_pos.second.x * 1.5f, joystick_pos.second.y * 1.5f);
            camera.rotate(joystick_pos.first.x * move_amount * 5.0f);
            camera.moveFront(-joystick_pos.first.y * move_amount);
        }
        if (eh.is_key_pressed(Key::MouseLeft))
        {
            if (!SDL_GetRelativeMouseMode()) SDL_SetRelativeMouseMode(SDL_TRUE);
            camera.onMouseMove(eh.mouse_motion.x * 1.5f, eh.mouse_motion.y * 1.5f);
            eh.mouse_motion = glm::vec2(0.0f);
        }
        if (eh.is_key_released(Key::MouseLeft))
        {
            SDL_SetRelativeMouseMode(SDL_FALSE);
            SDL_WarpMouseInWindow(vmc.window->get(), extent.width / 2.0f, extent.height / 2.0f);
            eh.set_released_key(Key::MouseLeft, false);
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
