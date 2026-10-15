#ifndef PENGUIN_LAYOUT_RUNTIME_H
#define PENGUIN_LAYOUT_RUNTIME_H
#include "layout/ui_layout.h"
#include "penguin/ui_sdl.h"

void editor_apply_shell_layout(void);
void editor_draw_global_tools(void);
void editor_draw_yaml_node(const cJSON *n, SDL_FRect area, int column);
void editor_draw_yaml_toolbar(SDL_FRect area);
const cJSON *editor_layout_entry(UiId id);
PeUiLayout *editor_load_editor_layout(UiId id, char *error, size_t size);
void editor_yaml_action(const char *name);
bool editor_yaml_action_active(const char *action);
bool editor_yaml_action_enabled(const char *action);
bool editor_yaml_boolean(const cJSON *value, bool fallback);
UiIcon editor_yaml_icon(const char *name);
const char *editor_yaml_text(const cJSON *value, char *buffer, size_t size);
int editor_yaml_toolbar_rows(float width);
#endif
