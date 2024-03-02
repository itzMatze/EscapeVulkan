#include "MainContext.hpp"

MainContext::MainContext(const std::string& nn_file, bool train_mode, bool disable_rendering) : extent(2000, 1500), vmc(extent.width, extent.height), vcc(vmc), wc(vmc, vcc), camera(60.0f, extent.width, extent.height), gs{.cam = camera}, agent(train_mode)
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
    for (uint32_t i = 0; i < gs.collision_results.distances.size(); ++i) state.push_back(gs.collision_results.distances[i]);
    state.push_back(velocity);
    state.push_back(rotation_speed.x);
    state.push_back(rotation_speed.y);
    Agent::Action action = Agent::NO_MOVE;
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
        if (!agent.is_training()) sound_player.set_volume(0, velocity + 40);
        else action = agent.get_action(state);
        float old_distance = gs.tunnel_distance_travelled + gs.segment_distance_travelled;
        move_amount = gs.time_diff * move_speed;
        gs.cam.updateVP(gs.time_diff);
        dispatch_pressed_keys(eh, action);
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
                    gs.disable_rendering = true;
                    sound_player.stop(0);
                    sound_player.play(SoundPlayer::GAME_OVER, 1, 0);
                    std::cout << "Distance: " << gs.tunnel_distance_travelled + gs.segment_distance_travelled << std::endl;
                }
                else
                {
                    sound_player.play(SoundPlayer::SPACESHIP_CRASH, 1, 0);
                }
            }
        }
        gs.total_frames++;
        gs.time_diff = agent.is_training() ? 0.016667f : timer.restart();
        gs.time += gs.time_diff;
    }
    if (agent.is_training()) agent.save_to_file("nn.pt");
}

void MainContext::restart()
{
    min_velocity = 1.0f;
    rotation_speed = glm::vec3(0.0f);
    velocity = 1.0f;
    gs.tunnel_distance_travelled = 0.0f;
    gs.segment_distance_travelled = 0.0f;
    gs.time = 0.0f;
    gs.player_segment_position = 0;
    gs.first_segment_indices_idx = 0;
    gs.player_reset_blink_counter = 0;
    gs.show_player = true;
    gs.collision_results.collision_detected = 0;
    gs.player_lifes = 3;
    gs.current_frame = 0;
    gs.total_frames = 0;
    gs.cam.reset();
    wc.restart();
}

void MainContext::dispatch_pressed_keys(EventHandler& eh, const Agent::Action action)
{
    if (gs.player_lifes > 0)
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
        restart();
        gs.disable_rendering = false;
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


