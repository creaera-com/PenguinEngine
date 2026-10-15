#ifndef PENGUIN_APP_SHELL_H
#define PENGUIN_APP_SHELL_H
#include <SDL3/SDL.h>
#include <stdbool.h>

void editor_draw(void);
void editor_event(SDL_Event *e);
bool editor_ui_controls_hit(float x, float y);
#endif
