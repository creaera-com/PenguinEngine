#ifndef PENGUIN_EDITORS_SCRIPTS_PANELS_H
#define PENGUIN_EDITORS_SCRIPTS_PANELS_H
#include <SDL3/SDL.h>
#include <stdbool.h>

void editor_draw_script_content(void);
void editor_draw_script_inspector(void);
void editor_draw_script_tools(SDL_FRect area);
bool editor_script_action(const char *name);
#endif
