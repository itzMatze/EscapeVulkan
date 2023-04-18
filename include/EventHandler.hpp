#pragma once

#include <SDL.h>
#include <unordered_set>
#include <glm/vec2.hpp>

#include "imgui.h"

enum class Key
{
    W,
    A,
    S,
    D,
    Q,
    E,
    G,
    M,
    Plus,
    Minus,
    Left,
    Right,
    Up,
    Down,
    Shift,
    MouseLeft,
    MouseMiddle,
    MouseRight
};

class EventHandler
{
public:
    glm::vec2 mouse_motion;
    std::unordered_set<Key> pressed_keys;
    std::unordered_set<Key> released_keys;

    EventHandler();
    void dispatch_event(SDL_Event e);

private:
    ImGuiIO& io;
    void apply_key_event(Key k, uint32_t et);
};