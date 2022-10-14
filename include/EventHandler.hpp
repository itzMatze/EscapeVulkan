#pragma once

#include <SDL.h>
#include <unordered_set>
#include <glm/vec2.hpp>

enum class Key
{
    W,
    A,
    S,
    D,
    Q,
    E,
    Plus,
    Minus,
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

    EventHandler() = default;
    void dispatch_event(SDL_Event e);

private:
    void apply_key_event(Key k, uint32_t et);
};