/** @file @brief Private shared capture and geometry helpers; no SDL required. */
#ifndef PENGUI_CORE_INTERNAL_H
#define PENGUI_CORE_INTERNAL_H
#include "pengui/pengui.h"
#include <math.h>

static inline float ui_clamp(float value, float low, float high) {
    return fmaxf(low, fminf(high, value));
}

bool ui_context_visible(const UiContext *context, float x, float y);
bool ui_context_acquire(UiContext *context, UiId id, UiRect area);
#endif
