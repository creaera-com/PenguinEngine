#ifndef PENGUIN_UI_LAYOUT_H
#define PENGUIN_UI_LAYOUT_H
#include "cJSON.h"
#include <stdbool.h>
#include <stddef.h>
#define PE_UI_MAX_NODES 2048
#define PE_UI_MAX_DEPTH 32
#define PE_UI_MAX_FILE (256 * 1024)
typedef struct {
    const cJSON *value;
    size_t line, column;
} PeUiMark;
typedef struct {
    cJSON *root;
    char path[1024];
    PeUiMark *marks;
    int mark_count;
} PeUiLayout;
PeUiLayout *pe_ui_layout_load(const char *path, const char *kind, char *error, size_t size);
void pe_ui_layout_free(PeUiLayout *layout);
const cJSON *pe_ui_get(const cJSON *object, const char *key);
const char *pe_ui_string(const cJSON *object, const char *key, const char *fallback);
const char *pe_ui_scalar(const cJSON *value, const char *fallback);
bool pe_ui_action_known(const char *name);
bool pe_ui_binding_known(const char *name);
#endif
