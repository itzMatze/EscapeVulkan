#include <SDL2/SDL_mixer.h>

#include "EventHandler.hpp"
#include "vk/common.hpp"
#include "vk/VulkanMainContext.hpp"
#include "vk/VulkanCommandContext.hpp"
#include "WorkContext.hpp"
#include "Agent.hpp"

class MainContext
{
public:
    MainContext(const std::string& nn_file = "", bool train_mode = false, bool disable_rendering = false);
    ~MainContext();
    void run();

private:
    Mix_Chunk* spaceship_sound = nullptr;
    Mix_Chunk* crash_sound = nullptr;
    Mix_Chunk* game_over_sound = nullptr;
    vk::Extent2D extent;
    ve::VulkanMainContext vmc;
    ve::VulkanCommandContext vcc;
    ve::WorkContext wc;
    Camera camera;
    float move_amount;
    float move_speed = 20.0f;
    float velocity = 0.0f;
    float min_velocity = 1.0f;
    glm::vec3 rotation_speed = glm::vec3(0.0f);
    ve::GameState gs;
    Agent agent;
    bool use_agent = false;
    bool game_mode = true;

    void restart();
    void dispatch_pressed_keys(EventHandler& eh, const Agent::Action action = Agent::NO_MOVE);
};
