#include <optional>

#include "EventHandler.hpp"
#include "vk/common.hpp"
#include "vk/VulkanMainContext.hpp"
#include "vk/VulkanCommandContext.hpp"
#include "WorkContext.hpp"
#include "Steering.hpp"
#include "SoundPlayer.hpp"
#include "Agent.hpp"
#include "MoveActions.hpp"

class MainContext
{
public:
    MainContext(const std::string& nn_file = "", bool train_mode = false, bool disable_rendering = false);
    ~MainContext();
    void run();

private:
    vk::Extent2D extent;
    ve::VulkanMainContext vmc;
    ve::VulkanCommandContext vcc;
    ve::WorkContext wc;
    Camera camera;
    ve::GameState gs;
    Steering::Simulation simulation_steering;
    std::optional<Steering::FreeFlight> free_flight_steering;
    SoundPlayer sound_player;
    Agent agent;
    bool use_agent = false;
    bool game_mode = true;

    void restart();
    void dispatch_pressed_keys(EventHandler& eh, const MoveActionFlags::type action = MoveActionFlags::NoMove);
};
