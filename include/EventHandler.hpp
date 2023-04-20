#pragma once

#include <SDL.h>
#include <vector>
#include <glm/vec2.hpp>

#include "imgui.h"

enum class Key
{
    W = 0,
    A = 1,
    S = 2,
    D = 3,
    Q = 4,
    E = 5,
    G = 6,
    M = 7,
    Plus = 8,
    Minus = 9,
    Left = 10,
    Right = 11,
    Up = 12,
    Down = 13,
    Shift = 14,
    MouseLeft = 15,
    MouseMiddle = 16,
    MouseRight = 17,
    Size = 18
};

class EventHandler
{
public:
    glm::vec2 mouse_motion;

    EventHandler();
    void dispatch_event(SDL_Event e);
    bool is_key_pressed(Key key) const;
    bool is_key_released(Key key) const;
    void set_pressed_key(Key key, bool value);
    void set_released_key(Key key, bool value);

private:
    ImGuiIO& io;
    std::vector<bool> pressed_keys;
    std::vector<bool> released_keys;
    void apply_key_event(Key k, uint32_t et);
    static uint32_t get_idx(Key key);
};