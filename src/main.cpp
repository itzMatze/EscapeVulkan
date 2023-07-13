#include <iostream>
#include <filesystem>
#include <stdexcept>
#include <thread>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include "vk/common.hpp"
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
    MainContext() : extent(1000, 800), vmc(extent.width, extent.height), vcc(vmc), wc(vmc, vcc), camera(60.0f, extent.width, extent.height), gs{.cam = camera}
    {
        gs.devicetimings.resize(ve::DeviceTimer::TIMER_COUNT, 0.0f);
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
        gs.current_scene = 0;
        for (const auto& entry : std::filesystem::directory_iterator("../assets/scenes/"))
        {
            if (entry.path().filename() == "escapevulkan.json") gs.current_scene = scene_names.size();
            scene_names.push_back(entry.path().filename());
        }
        for (const auto& name : scene_names) gs.scene_names.push_back(&name.front());
        wc.load_scene(gs.scene_names[gs.current_scene]);
        constexpr float min_frametime = 5.0f;
        // keep time measurement and frametime separate to be able to use a frame limiter
        ve::HostTimer timer;
        bool quit = false;
        SDL_Event e;
        while (!quit)
        {
            move_amount = gs.time_diff * move_speed;
            dispatch_pressed_keys();
            gs.cam.updateVP();
            try
            {
                //std::this_thread::sleep_for(std::chrono::duration<float, std::milli>(min_frametime - gs.frametime));
                gs.time_diff /= 1000.0f;
                gs.player_pos = camera.getPosition();
                wc.draw_frame(gs);
                if (gs.player_lifes == 0) quit = true;
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
            gs.time_diff = timer.restart();
            gs.time += gs.time_diff;
            // calculate actual frametime by subtracting the waiting time
            //gs.frametime = gs.time_diff - std::max(0.0f, min_frametime - gs.frametime);
            if (gs.load_scene)
            {
                gs.load_scene = false;
                wc.load_scene(gs.scene_names[gs.current_scene]);
                timer.restart();
            }
        }
        std::cout << "Distance: " << gs.tunnel_distance_travelled << std::endl;
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
    float velocity = 1.0f;
    glm::vec3 rotation_speed;
    ve::GameState gs;
    bool game_mode = false;

    void dispatch_pressed_keys()
    {
        if(game_mode)
        {
            if (eh.is_key_pressed(Key::Left)) rotation_speed.x -= 0.0004f * gs.time_diff;
            if (eh.is_key_pressed(Key::Right)) rotation_speed.x += 0.0004f * gs.time_diff;
            if (eh.is_key_pressed(Key::Up)) rotation_speed.y -= 0.0004f * gs.time_diff;
            if (eh.is_key_pressed(Key::Down)) rotation_speed.y += 0.0004f * gs.time_diff;
            if (eh.is_key_pressed(Key::A)) camera.rotate(0.1f * gs.time_diff);//rotation_speed.z -= 0.002f;
            if (eh.is_key_pressed(Key::D)) camera.rotate(-0.1f * gs.time_diff);//rotation_speed.z += 0.002f;
            if (eh.is_key_pressed(Key::W)) velocity += 0.002f * gs.time_diff;
            if (eh.is_key_pressed(Key::S)) velocity -= 0.002f * gs.time_diff;
            velocity = std::max(1.0f, velocity);
            if (gs.player_reset_blink_counter > 0)
            {
                velocity = 0.0f;
                rotation_speed = glm::vec3(0.0f);
            }
            gs.tunnel_distance_travelled += move_amount * velocity;
            camera.moveFront(move_amount * velocity);
            camera.onMouseMove(rotation_speed.x * gs.time_diff, 0.0f);
            camera.onMouseMove(0.0f, rotation_speed.y * gs.time_diff);
        }
        else
        {
            if (eh.is_key_pressed(Key::W)) camera.moveFront(move_amount);
            if (eh.is_key_pressed(Key::A)) camera.moveRight(-move_amount);
            if (eh.is_key_pressed(Key::S)) camera.moveFront(-move_amount);
            if (eh.is_key_pressed(Key::D)) camera.moveRight(move_amount);
            if (eh.is_key_pressed(Key::Q)) camera.moveDown(move_amount);
            if (eh.is_key_pressed(Key::E)) camera.moveDown(-move_amount);
            float panning_speed = eh.is_key_pressed(Key::Shift) ? 0.2f : 0.6f;
            if (eh.is_key_pressed(Key::Left)) camera.onMouseMove(-panning_speed * gs.time_diff, 0.0f);
            if (eh.is_key_pressed(Key::Right)) camera.onMouseMove(panning_speed * gs.time_diff, 0.0f);
            if (eh.is_key_pressed(Key::Up)) camera.onMouseMove(0.0f, -panning_speed * gs.time_diff);
            if (eh.is_key_pressed(Key::Down)) camera.onMouseMove(0.0f, panning_speed * gs.time_diff);
        }

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
            gs.show_ui = !gs.show_ui;
            eh.set_released_key(Key::G, false);
        }
        if (eh.is_key_released(Key::M))
        {
            gs.mesh_view = !gs.mesh_view;
            eh.set_released_key(Key::M, false);
        }
        if (eh.is_key_released(Key::N))
        {
            gs.normal_view = !gs.normal_view;
            eh.set_released_key(Key::N, false);
        }
        if (eh.is_key_released(Key::T))
        {
            gs.tex_view = !gs.tex_view;
            eh.set_released_key(Key::T, false);
        }
        if (eh.is_key_released(Key::C))
        {
            gs.color_view = !gs.color_view;
            eh.set_released_key(Key::C, false);
        }
        if (eh.is_key_released(Key::P))
        {
            gs.collision_detection_active = !gs.collision_detection_active;
            eh.set_released_key(Key::P, false);
        }
        if (eh.is_key_released(Key::B))
        {
            gs.show_player_bb = !gs.show_player_bb;
            eh.set_released_key(Key::B, false);
        }
        if (eh.is_key_released(Key::F))
        {
            camera.is_tracking_camera = !camera.is_tracking_camera;
            eh.set_released_key(Key::F, false);
        }
        if (eh.is_key_released(Key::F1))
        {
            gs.save_screenshot = true;
            eh.set_released_key(Key::F1, false);
        }
        if (eh.is_key_released(Key::X))
        {
            game_mode = !game_mode;
            eh.set_released_key(Key::X, false);
        }
        if (eh.is_key_released(Key::R))
        {
            system("cmake --build . --target Shaders");
            eh.set_released_key(Key::R, false);
            wc.reload_shaders();
        }
        if (game_mode && eh.is_controller_available())
        {
            std::pair<glm::vec2, glm::vec2> joystick_pos = eh.get_controller_joystick_pos();
            rotation_speed.x += 0.0004f * joystick_pos.second.x * gs.time_diff;
            rotation_speed.y += 0.0004f * joystick_pos.second.y * gs.time_diff;
            velocity -= 0.002f * joystick_pos.first.y * gs.time_diff;
            camera.rotate(joystick_pos.first.x * move_amount * 5.0f);
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
