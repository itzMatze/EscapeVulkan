#include "MainContext.hpp"
#include "MoveActions.hpp"
#include "vk/common.hpp"
#include "Camera.hpp"

MainContext::MainContext(const std::string& nn_file, bool train_mode, bool disable_rendering) : extent(2000, 1500), vmc(extent.width, extent.height), vcc(vmc), wc(vmc, vcc), camera(60.0f, extent.width, extent.height), gs{.cam = camera}, agent(train_mode)
{
    gs.session_data.devicetimings.resize(ve::DeviceTimer::TIMER_COUNT, 0.0f);
    extent = wc.swapchain.get_extent();
    camera.updateScreenSize(extent.width, extent.height);
    gs.settings.disable_rendering = disable_rendering;
    if (nn_file != "")
    {
        use_agent = true;
        agent.load_from_file(nn_file);
    }
}

MainContext::~MainContext()
{
    wc.self_destruct();
    vcc.self_destruct();
    vmc.self_destruct();
    spdlog::info("Destroyed MainContext");
}

void MainContext::run()
{
    EventHandler eh;
    wc.load_scene("escapevulkan.json");
    std::vector<float> state;
    for (uint32_t i = 0; i < gs.game_data.collision_results.distances.size(); ++i) state.push_back(gs.game_data.collision_results.distances[i]);
    state.push_back(simulation_steering.get_velocity());
    state.push_back(simulation_steering.get_rotation_speed().x);
    state.push_back(simulation_steering.get_rotation_speed().y);
    MoveActionFlags::type action = MoveActionFlags::NoMove;
    uint32_t iteration = 0;
    if (!agent.is_training())
    {
        sound_player.init();
        sound_player.set_volume(0, 0);
        sound_player.play(SoundPlayer::SPACESHIP_THRUST, 0, -1);
    }

    ve::HostTimer timer;
    bool quit = false;
    SDL_Event e;
    while (!quit)
    {
        if (!agent.is_training()) sound_player.set_volume(0, simulation_steering.get_velocity() + 40);
        else action = agent.get_action(state);
        float old_distance = gs.game_data.tunnel_distance_travelled + gs.game_data.segment_distance_travelled;
        gs.cam.updateVP(gs.game_data.time_diff);
        dispatch_pressed_keys(eh, action);
        uint32_t old_player_lifes = gs.game_data.player_lifes;
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
            for (uint32_t i = 0; i < gs.game_data.collision_results.distances.size(); ++i)
            {
                state[i] = gs.game_data.collision_results.distances[i];
            }
            float new_distance = gs.game_data.tunnel_distance_travelled + gs.game_data.segment_distance_travelled;
            reward += new_distance - old_distance;
            agent.add_reward_for_last_action(reward);
            state[gs.game_data.collision_results.distances.size()] = simulation_steering.get_velocity();
            state[gs.game_data.collision_results.distances.size() + 1] = simulation_steering.get_rotation_speed().x;
            state[gs.game_data.collision_results.distances.size() + 2] = simulation_steering.get_rotation_speed().y;
        }
        if (gs.game_data.player_lifes < old_player_lifes || (agent.is_training() && gs.game_data.total_frames > 1200))
        {
            if (agent.is_training())
            {
                std::cout << "Distance: " << gs.game_data.tunnel_distance_travelled + gs.game_data.segment_distance_travelled << std::endl;
                std::cout << "Iteration: " << iteration++ << std::endl;
                agent.optimize();
                restart();
            }
            else
            {
                std::cout << "Lifes: " << gs.game_data.player_lifes << std::endl;
                if (gs.game_data.player_lifes == 0)
                {
                    gs.settings.disable_rendering = true;
                    sound_player.stop(0);
                    sound_player.play(SoundPlayer::GAME_OVER, 1, 0);
                    std::cout << "Distance: " << gs.game_data.tunnel_distance_travelled + gs.game_data.segment_distance_travelled << std::endl;
                }
                else
                {
                    sound_player.play(SoundPlayer::SPACESHIP_CRASH, 1, 0);
                }
            }
        }
        gs.game_data.total_frames++;
        gs.game_data.time_diff = agent.is_training() ? 0.016667f : timer.restart();
        gs.game_data.time += gs.game_data.time_diff;
    }
    if (agent.is_training()) agent.save_to_file("nn.pt");
}

void MainContext::restart()
{
    gs.game_data = ve::GameData();
    gs.cam.reset();
    simulation_steering.reset();
    wc.restart();
}

void MainContext::dispatch_pressed_keys(EventHandler& eh, const MoveActionFlags::type action)
{
    if (gs.game_data.player_lifes > 0)
    {
        MoveActionFlags::type move_actions = action;
        Steering::Move move;
        if (eh.is_key_pressed(Key::W)) move_actions |= MoveActionFlags::MoveForward;
        if (eh.is_key_pressed(Key::S)) move_actions |= MoveActionFlags::MoveBackward;
        if (eh.is_key_pressed(Key::Left)) move_actions |= MoveActionFlags::RotateLeft;
        if (eh.is_key_pressed(Key::Right)) move_actions |= MoveActionFlags::RotateRight;
        if (eh.is_key_pressed(Key::Up)) move_actions |= MoveActionFlags::RotateUp;
        if (eh.is_key_pressed(Key::Down)) move_actions |= MoveActionFlags::RotateDown;
        if(game_mode)
        {
            if (gs.game_data.player_reset_blink_counter > 0)
            {
                simulation_steering.reset();
                move_actions = MoveActionFlags::NoMove;
            }
            // enable controller if found, always permit keyboard steering
            if (eh.is_controller_available())
            {
                std::pair<glm::vec2, glm::vec2> joystick_pos = eh.get_controller_joystick_pos();
                simulation_steering.controller_input(joystick_pos, gs.game_data.time_diff, move);
            }
            if (eh.is_key_pressed(Key::A)) move_actions |= MoveActionFlags::RollLeft;
            if (eh.is_key_pressed(Key::D)) move_actions |= MoveActionFlags::RollRight;
            simulation_steering.keyboard_input(move_actions, gs.game_data.time_diff, move);
            simulation_steering.step(gs.game_data.time_diff, move);
        }
        else
        {
            if (!free_flight_steering.has_value()) free_flight_steering = std::make_optional(Steering::FreeFlight());
            if (eh.is_key_pressed(Key::A)) move_actions |= MoveActionFlags::MoveLeft;
            if (eh.is_key_pressed(Key::D)) move_actions |= MoveActionFlags::MoveRight;
            if (eh.is_key_pressed(Key::Q)) move_actions |= MoveActionFlags::MoveUp;
            if (eh.is_key_pressed(Key::E)) move_actions |= MoveActionFlags::MoveDown;
            free_flight_steering->keyboard_input(move_actions, gs.game_data.time_diff, move);
        }
        camera.moveRight(move.position_delta.x);
        camera.moveDown(move.position_delta.y);
        camera.moveFront(move.position_delta.z);
        camera.onMouseMove(move.rotation_delta.x, move.rotation_delta.y);
        camera.rotate(move.rotation_delta.z);

        // reset state of keys that are used to execute a one time action
        if (eh.is_key_released(Key::Plus))
        {
            if (free_flight_steering.has_value()) free_flight_steering->increase_speed();
            eh.set_released_key(Key::Plus, false);
        }
        if (eh.is_key_released(Key::Minus))
        {
            if (free_flight_steering.has_value()) free_flight_steering->reduce_speed();
            eh.set_released_key(Key::Minus, false);
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
    if (eh.is_key_released(Key::G))
    {
        gs.settings.show_ui = !gs.settings.show_ui;
        eh.set_released_key(Key::G, false);
    }
    if (eh.is_key_released(Key::M))
    {
        gs.settings.mesh_view = !gs.settings.mesh_view;
        eh.set_released_key(Key::M, false);
    }
    if (eh.is_key_released(Key::N))
    {
        gs.settings.normal_view = !gs.settings.normal_view;
        eh.set_released_key(Key::N, false);
    }
    if (eh.is_key_released(Key::T))
    {
        gs.settings.tex_view = !gs.settings.tex_view;
        eh.set_released_key(Key::T, false);
    }
    if (eh.is_key_released(Key::C))
    {
        gs.settings.color_view = !gs.settings.color_view;
        eh.set_released_key(Key::C, false);
    }
    if (eh.is_key_released(Key::P))
    {
        gs.settings.collision_detection_active = !gs.settings.collision_detection_active;
        eh.set_released_key(Key::P, false);
    }
    if (eh.is_key_released(Key::B))
    {
        gs.settings.show_player_bb = !gs.settings.show_player_bb;
        eh.set_released_key(Key::B, false);
    }
    if (eh.is_key_released(Key::F))
    {
        camera.is_tracking_camera = !camera.is_tracking_camera;
        eh.set_released_key(Key::F, false);
    }
    if (eh.is_key_released(Key::F1))
    {
        gs.settings.save_screenshot = true;
        eh.set_released_key(Key::F1, false);
    }
    if (eh.is_key_released(Key::F2))
    {
        restart();
        gs.settings.disable_rendering = false;
        sound_player.play(SoundPlayer::SPACESHIP_THRUST, 0, -1);
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
}


