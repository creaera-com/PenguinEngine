/** @file @brief Keep native modifier handling separate from reusable geometry. */
#include "modifiers.h"

bool vector_transform_modifiers(const SDL_Event *event, bool *aspect, bool *centered) {
    bool key = event->type == SDL_EVENT_KEY_DOWN || event->type == SDL_EVENT_KEY_UP;
    if (key && event->key.key != SDLK_LSHIFT && event->key.key != SDLK_RSHIFT &&
        event->key.key != SDLK_LALT && event->key.key != SDLK_RALT) {
        return false;
    }
    if (key || event->type == SDL_EVENT_MOUSE_MOTION ||
        event->type == SDL_EVENT_MOUSE_BUTTON_DOWN || event->type == SDL_EVENT_MOUSE_BUTTON_UP) {
        SDL_Keymod mods = key ? event->key.mod : SDL_GetModState();
        *aspect = (mods & SDL_KMOD_SHIFT) != 0;
        *centered = (mods & SDL_KMOD_ALT) != 0;
    }
    return key;
}
