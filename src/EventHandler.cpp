#include "EventHandler.hpp"

void EventHandler::dispatch_event(SDL_Event e)
{
    if (e.type == SDL_MOUSEMOTION)
    {
        mouse_motion.x = e.motion.xrel;
        mouse_motion.y = e.motion.yrel;
    }
    switch (e.button.button)
    {
        case SDL_BUTTON_LEFT:
            apply_key_event(Key::MouseLeft, e.type);
            break;
        case SDL_BUTTON_MIDDLE:
            apply_key_event(Key::MouseMiddle, e.type);
            break;
        case SDL_BUTTON_RIGHT:
            apply_key_event(Key::MouseRight, e.type);
            break;
    }
    switch (e.key.keysym.sym)
    {
        case SDLK_w:
            apply_key_event(Key::W, e.type);
            break;
        case SDLK_a:
            apply_key_event(Key::A, e.type);
            break;
        case SDLK_s:
            apply_key_event(Key::S, e.type);
            break;
        case SDLK_d:
            apply_key_event(Key::D, e.type);
            break;
        case SDLK_q:
            apply_key_event(Key::Q, e.type);
            break;
        case SDLK_e:
            apply_key_event(Key::E, e.type);
            break;
        case SDLK_PLUS:
            apply_key_event(Key::Plus, e.type);
            break;
        case SDLK_MINUS:
            apply_key_event(Key::Minus, e.type);
            break;
    }
}

void EventHandler::apply_key_event(Key k, uint32_t et)
{
    if (et == SDL_KEYDOWN || et == SDL_MOUSEBUTTONDOWN)
    {
        pressed_keys.insert(k);
        released_keys.erase(k);
    }
    else if (et == SDL_KEYUP || et == SDL_MOUSEBUTTONUP)
    {
        pressed_keys.erase(k);
        released_keys.insert(k);
    }
}
