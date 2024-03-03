#include <utility>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include "MoveActions.hpp"

namespace Steering
{
struct Move
{
    glm::vec3 rotation_delta = glm::vec3(0.0f);
    glm::vec3 position_delta = glm::vec3(0.0f);
};

class Simulation
{
public:
    Simulation();
    void reset();
    void step(float time_diff, Move& move);
    void controller_input(std::pair<glm::vec2, glm::vec2>& joystick_pos, float time_diff, Move& move);
    void keyboard_input(MoveActionFlags::type actions, float time_diff, Move& move);
    float get_velocity();
    glm::vec3 get_rotation_speed();
private:
    float move_speed = 20.0f;
    float velocity = 0.0f;
    float min_velocity = 1.0f;
    glm::vec3 rotation_speed = glm::vec3(0.0f);
};

class FreeFlight
{
public:
    FreeFlight() = default;
    void increase_speed();
    void reduce_speed();
    void keyboard_input(MoveActionFlags::type actions, float time_diff, Move& move);
private:
    float move_speed = 20.0f;
};
}

