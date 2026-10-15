#ifndef PENGUIN_APP_UI_HELPERS_H
#define PENGUIN_APP_UI_HELPERS_H
#include "penguin/ui_sdl.h"

void editor_box(SDL_FRect r, uint32_t c);
bool editor_button_impl(UiId callsite, float x, float y, float w, const char *label, bool active,
                        bool enabled);
void editor_disclosure(float x, float y, float w, const char *title, bool *open);
void editor_draw_color(uint32_t c);
void editor_draw_split_grip(UiRect area, bool highlighted);
bool editor_icon_button_impl(UiId callsite, float x, float y, float w, UiIcon icon,
                             const char *label, const char *hint, bool active, bool enabled);
bool editor_inside(float x, float y, SDL_FRect r);
void editor_register_ui_hit(UiRect rect);
void editor_section(float x, float y, UiIcon icon, const char *title);
void editor_text(float x, float y, const char *label, uint32_t color);
#endif
