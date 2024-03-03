#include "Steering.hpp"

#include <algorithm>

#include "MoveActions.hpp"

namespace Steering {
Simulation::Simulation()
{
    reset();
}

void Simulation::reset()
{
    move_speed = 20.0f;
    min_velocity = 1.0f;
    rotation_speed = glm::vec3(0.0f);
    velocity = 1.0f;
}

void Simulation::step(float time_diff, Move& move)
{
    min_velocity += 10.0f * time_diff;
    min_velocity = std::min(20.0f, min_velocity);
    velocity = std::max(min_velocity, velocity);
    move.rotation_delta.x += rotation_speed.x * time_diff;
    move.rotation_delta.y += rotation_speed.y * time_diff;
    move.position_delta.z += velocity * time_diff;
}

void Simulation::controller_input(std::pair<glm::vec2, glm::vec2>& joystick_pos, float time_diff, Move& move)
{
    float move_amount = time_diff * move_speed;
    rotation_speed.x += 800.0f * joystick_pos.second.x * time_diff;
    rotation_speed.y += 800.0f * joystick_pos.second.y * time_diff;
    velocity -= 40.0f * joystick_pos.first.y * time_diff;
    move.rotation_delta.z += joystick_pos.first.x * move_amount * 5.0f;
}

void Simulation::keyboard_input(MoveActionFlags::type actions, float time_diff, Move& move)
{
    float move_amount = time_diff * move_speed;
    if (actions & MoveActionFlags::RotateLeft) rotation_speed.x -= 800.0f * time_diff;
    if (actions & MoveActionFlags::RotateRight) rotation_speed.x += 800.0f * time_diff;
    if (actions & MoveActionFlags::RotateUp) rotation_speed.y -= 800.0f * time_diff;
    if (actions & MoveActionFlags::RotateDown) rotation_speed.y += 800.0f * time_diff;
    if (actions & MoveActionFlags::RollLeft) move.rotation_delta.z += -5.0f * move_amount;//rotation_speed.z -= 0.002f;
    if (actions & MoveActionFlags::RollRight) move.rotation_delta.z += 5.0f * move_amount;//rotation_speed.z += 0.002f;
    if (actions & MoveActionFlags::MoveForward) velocity += 40.0f * time_diff;
    if (actions & MoveActionFlags::MoveBackward) velocity -= 40.0f * time_diff;
}

float Simulation::get_velocity()
{
    return velocity;
}
glm::vec3 Simulation::get_rotation_speed()
{
    return rotation_speed;
}

void FreeFlight::keyboard_input(MoveActionFlags::type actions, float time_diff, Move& move)
{
    float move_amount = time_diff * move_speed;
    if (actions & MoveActionFlags::RotateLeft) move.rotation_delta.x += -600.0f * time_diff;
    if (actions & MoveActionFlags::RotateRight) move.rotation_delta.x += 600.0f * time_diff;
    if (actions & MoveActionFlags::RotateUp) move.rotation_delta.y += -600.0f * time_diff;
    if (actions & MoveActionFlags::RotateDown) move.rotation_delta.y += 600.0f * time_diff;
    if (actions & MoveActionFlags::MoveLeft) move.position_delta.x += -1.0f * move_amount;//rotation_speed.z -= 0.002f;
    if (actions & MoveActionFlags::MoveRight) move.position_delta.x += 1.0f * move_amount;//rotation_speed.z += 0.002f;
    if (actions & MoveActionFlags::MoveForward) move.position_delta.z += 1.0f * move_amount;
    if (actions & MoveActionFlags::MoveBackward) move.position_delta.z += -1.0f * move_amount;
    if (actions & MoveActionFlags::MoveUp) move.position_delta.y += -1.0f * move_amount;
    if (actions & MoveActionFlags::MoveDown) move.position_delta.y += 1.0f * move_amount;
}

void FreeFlight::increase_speed()
{
    move_speed *= 2;
}

void FreeFlight::reduce_speed()
{
    move_speed /= 2;
}
}

