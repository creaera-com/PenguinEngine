/** @file @brief SDL-free internal color-picker state and composite layout expansion. */
#ifndef PUI_COLOR_PICKER_INTERNAL_H
#define PUI_COLOR_PICKER_INTERNAL_H
#include "pengui/pengui.h"

typedef struct {
    PUIHSLColor hsla;
    uint32_t rgba, committed;
    int mode, drag;
    bool notify_preview, notify_commit, invalid;
} PUIColorPickerState;

/** @brief Add the nine ordinary text fields owned by a color-picker node. */
bool pui_color_picker_expand(PUILayout *layout, int index, char *error, size_t capacity);
#endif
