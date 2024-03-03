#pragma once

#include <SDL_events.h>
#include <SDL_gamecontroller.h>
#include <vector>
#include <glm/vec2.hpp>

#include "imgui.h"

enum class Key : uint32_t
{
    A = 0,
    B = 1,
    C = 2,
    D = 3,
    E = 4,
    F = 5,
    G = 6,
    H = 7,
    I = 8,
    J = 9,
    K = 10,
    L = 11,
    M = 12,
    N = 13,
    O = 14,
    P = 15,
    Q = 16,
    R = 17,
    S = 18,
    T = 19,
    U = 20,
    V = 21,
    W = 22,
    X = 23,
    Y = 24,
    Z = 25,
    Plus = 26,
    Minus = 27,
    Left = 28,
    Right = 29,
    Up = 30,
    Down = 31,
    Shift = 32,
    MouseLeft = 33,
    MouseMiddle = 34,
    MouseRight = 35,
    F1 = 36,
    F2 = 37,
    Size = 38
};

class EventHandler
{
public:
    glm::vec2 mouse_motion;

    EventHandler();
    void dispatch_event(SDL_Event e);
    std::pair<glm::vec2, glm::vec2> get_controller_joystick_pos();
    bool is_controller_available();
    bool is_key_pressed(Key key) const;
    bool is_key_released(Key key) const;
    void set_pressed_key(Key key, bool value);
    void set_released_key(Key key, bool value);

private:
    ImGuiIO& io;
    std::vector<bool> pressed_keys;
    std::vector<bool> released_keys;
    SDL_GameController* controller = nullptr;
    void apply_key_event(Key k, uint32_t et);
    static uint32_t get_idx(Key key);
};
