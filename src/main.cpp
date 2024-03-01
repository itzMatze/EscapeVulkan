#include <boost/program_options/options_description.hpp>
#include <boost/program_options/value_semantic.hpp>
#include <boost/program_options/variables_map.hpp>
#include <iostream>
#include <boost/program_options.hpp>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <SDL2/SDL_mixer.h>

#include "vk/common.hpp"
#include "Camera.hpp"
#include "EventHandler.hpp"
#include "ve_log.hpp"
#include "vk/Timer.hpp"
#include "vk/VulkanMainContext.hpp"
#include "vk/VulkanCommandContext.hpp"
#include "WorkContext.hpp"
#include "Agent.hpp"

class MainContext
{
public:
    MainContext(const std::string& nn_file = "", bool train_mode = false, bool disable_rendering = false) : extent(2000, 1500), vmc(extent.width, extent.height), vcc(vmc), wc(vmc, vcc), camera(60.0f, extent.width, extent.height), gs{.cam = camera}, agent(train_mode)
    {
        gs.devicetimings.resize(ve::DeviceTimer::TIMER_COUNT, 0.0f);
        extent = wc.swapchain.get_extent();
        camera.updateScreenSize(extent.width, extent.height);
        gs.disable_rendering = disable_rendering;
        if (nn_file != "")
        {
            use_agent = true;
            agent.load_from_file(nn_file);
        }
    }

    ~MainContext()
    {
        Mix_FreeChunk(spaceship_sound);
        Mix_FreeChunk(crash_sound);
        Mix_FreeChunk(game_over_sound);
        spaceship_sound = nullptr;
        crash_sound = nullptr;
        game_over_sound = nullptr;
        Mix_CloseAudio();
        wc.self_destruct();
        vcc.self_destruct();
        vmc.self_destruct();
        spdlog::info("Destroyed MainContext");
    }

    void run()
    {
        wc.load_scene("escapevulkan.json");
        std::vector<float> state;
        for (uint32_t i = 0; i < gs.collision_results.distances.size(); ++i) state.push_back(gs.collision_results.distances[i]);
        state.push_back(velocity);
        state.push_back(rotation_speed.x);
        state.push_back(rotation_speed.y);
        Agent::Action action = Agent::NO_MOVE;
        uint32_t iteration = 0;
        if (!agent.is_training())
        {
            // initialize mixer
            Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 2, 4096);
            spaceship_sound = Mix_LoadWAV("../assets/sounds/spaceship_thrust_1.wav");
            crash_sound = Mix_LoadWAV("../assets/sounds/spaceship_crash.wav");
            game_over_sound = Mix_LoadWAV("../assets/sounds/game_over.wav");

            Mix_PlayChannel(0, spaceship_sound, -1);
            Mix_Volume(1, MIX_MAX_VOLUME);
        }

        ve::HostTimer timer;
        bool quit = false;
        SDL_Event e;
        while (!quit)
        {
            if (!agent.is_training()) Mix_Volume(0, velocity + 40);
            else action = agent.get_action(state);
            float old_distance = gs.tunnel_distance_travelled + gs.segment_distance_travelled;
            move_amount = gs.time_diff * move_speed;
            gs.cam.updateVP(gs.time_diff);
            dispatch_pressed_keys(action);
            uint32_t old_player_lifes = gs.player_lifes;
            try
            {
                //std::this_thread::sleep_for(std::chrono::duration<float, std::milli>(min_frametime - gs.frametime));
                wc.draw_frame(gs);
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
            if (agent.is_training())
            {
                float reward = 1.0f;
                for (uint32_t i = 0; i < gs.collision_results.distances.size(); ++i)
                {
                    state[i] = gs.collision_results.distances[i];
                }
                float new_distance = gs.tunnel_distance_travelled + gs.segment_distance_travelled;
                reward += new_distance - old_distance;
                agent.add_reward_for_last_action(reward);
                state[gs.collision_results.distances.size()] = velocity;
                state[gs.collision_results.distances.size() + 1] = rotation_speed.x;
                state[gs.collision_results.distances.size() + 2] = rotation_speed.y;
            }
            if (gs.player_lifes < old_player_lifes || (agent.is_training() && gs.total_frames > 1200))
            {
                if (agent.is_training())
                {
                    std::cout << "Distance: " << gs.tunnel_distance_travelled + gs.segment_distance_travelled << std::endl;
                    std::cout << "Iteration: " << iteration++ << std::endl;
                    agent.optimize();
                    restart();
                }
                else
                {
                    std::cout << "Lifes: " << gs.player_lifes << std::endl;
                    if (gs.player_lifes == 0)
                    {
                        Mix_HaltChannel(0);
                        Mix_PlayChannel(1, game_over_sound, 0);
                        std::cout << "Distance: " << gs.tunnel_distance_travelled + gs.segment_distance_travelled << std::endl;
                    }
                    else
                    {
                        Mix_PlayChannel(1, crash_sound, 0);
                    }
                }
            }
            gs.total_frames++;
            gs.time_diff = agent.is_training() ? 0.016667f : timer.restart();
            gs.time += gs.time_diff;
        }
        if (agent.is_training()) agent.save_to_file("nn.pt");
    }

private:
    Mix_Chunk* spaceship_sound = nullptr;
    Mix_Chunk* crash_sound = nullptr;
    Mix_Chunk* game_over_sound = nullptr;
    vk::Extent2D extent;
    ve::VulkanMainContext vmc;
    ve::VulkanCommandContext vcc;
    ve::WorkContext wc;
    Camera camera;
    EventHandler eh;
    float move_amount;
    float move_speed = 20.0f;
    float velocity = 1.0f;
    float min_velocity = 1.0f;
    glm::vec3 rotation_speed = glm::vec3(0.0f);
    ve::GameState gs;
    Agent agent;
    bool use_agent = false;
    bool game_mode = true;

    void restart()
    {
        min_velocity = 1.0f;
        rotation_speed = glm::vec3(0.0f);
        velocity = 1.0f;
        gs.tunnel_distance_travelled = 0.0f;
        gs.segment_distance_travelled = 0.0f;
        gs.time = 0.0f;
        gs.player_segment_position = 0;
        gs.first_segment_indices_idx = 0;
        gs.player_lifes = 3;
        gs.current_frame = 0;
        gs.total_frames = 0;
        gs.cam.reset();
        wc.restart();
    }

    void dispatch_pressed_keys(const Agent::Action action = Agent::NO_MOVE)
    {
        if(game_mode)
        {
            // enable controller if found, always permit keyboard steering
            if (eh.is_controller_available())
            {
                std::pair<glm::vec2, glm::vec2> joystick_pos = eh.get_controller_joystick_pos();
                rotation_speed.x += 800.0f * joystick_pos.second.x * gs.time_diff;
                rotation_speed.y += 800.0f * joystick_pos.second.y * gs.time_diff;
                velocity -= 40.0f * joystick_pos.first.y * gs.time_diff;
                camera.rotate(joystick_pos.first.x * move_amount * 5.0f);
            }
            if (eh.is_key_pressed(Key::Left) || (action & Agent::MOVE_LEFT)) rotation_speed.x -= 800.0f * gs.time_diff;
            if (eh.is_key_pressed(Key::Right) || (action & Agent::MOVE_RIGHT)) rotation_speed.x += 800.0f * gs.time_diff;
            if (eh.is_key_pressed(Key::Up) || (action & Agent::MOVE_UP)) rotation_speed.y -= 800.0f * gs.time_diff;
            if (eh.is_key_pressed(Key::Down) || (action & Agent::MOVE_DOWN)) rotation_speed.y += 800.0f * gs.time_diff;
            if (eh.is_key_pressed(Key::A) || (action & Agent::ROTATE_LEFT)) camera.rotate(-100.0f * gs.time_diff);//rotation_speed.z -= 0.002f;
            if (eh.is_key_pressed(Key::D) || (action & Agent::ROTATE_RIGHT)) camera.rotate(100.0f * gs.time_diff);//rotation_speed.z += 0.002f;
            if (eh.is_key_pressed(Key::W)) velocity += 40.0f * gs.time_diff;
            if (eh.is_key_pressed(Key::S)) velocity -= 40.0f * gs.time_diff;
            min_velocity += 10.0f * gs.time_diff;
            min_velocity = std::min(20.0f, min_velocity);
            if (gs.player_reset_blink_counter > 0) min_velocity = 1.0f;
            velocity = std::max(min_velocity, velocity);
            if (gs.player_reset_blink_counter > 0)
            {
                velocity = 0.0f;
                rotation_speed = glm::vec3(0.0f);
            }
            camera.moveFront(gs.time_diff * velocity);
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
            float panning_speed = eh.is_key_pressed(Key::Shift) ? 200.0f : 600.0f;
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
        if (eh.is_key_released(Key::F2))
        {
            wc.restart();
            Mix_PlayChannel(0, spaceship_sound, -1);
            eh.set_released_key(Key::F2, false);
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

int parse_args(int argc, char** argv, boost::program_options::variables_map& vm)
{
    namespace bpo = boost::program_options;
    bpo::options_description desc("Allowed options");
    desc.add_options()
        ("help", "Print this help message")
        ("nn_file", bpo::value<std::string>(), "Load neural network checkpoint from given file and initialize the agent with this network")
        ("train_mode,T", "Train agent")
        ("disable_rendering,R", "Do not render the game to enable quicker training iterations")
    ;
    bpo::store(bpo::parse_command_line(argc, argv, desc), vm);
    bpo::notify(vm);
    if (vm.count("help"))
    {
        std::cout << desc << std::endl;
        return 1;
    }
    return 0;
}

int main(int argc, char** argv)
{
    boost::program_options::variables_map vm;
    int parse_status = parse_args(argc, argv, vm);
    if (parse_status != 0) return parse_status;
    std::string nn_file = "";
    if (vm.count("nn_file")) nn_file = vm["nn_file"].as<std::string>();
    bool train_mode = vm.count("train_mode");
    bool disable_rendering = vm.count("disable_rendering");

    std::vector<spdlog::sink_ptr> sinks;
    sinks.push_back(std::make_shared<spdlog::sinks::stdout_sink_st>());
    sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_st>("ve.log", true));
    auto combined_logger = std::make_shared<spdlog::logger>("default_logger", sinks.begin(), sinks.end());
    spdlog::set_default_logger(combined_logger);
    spdlog::set_level(spdlog::level::debug);
    spdlog::set_pattern("[%Y-%m-%d %T.%e] [%L] %v");
    spdlog::info("Starting");
    auto t1 = std::chrono::high_resolution_clock::now();
    MainContext mc(nn_file, train_mode, disable_rendering);
    auto t2 = std::chrono::high_resolution_clock::now();
    spdlog::info("Setup took: {} ms", (std::chrono::duration<double, std::milli>(t2 - t1).count()));
    mc.run();
    return 0;
}
