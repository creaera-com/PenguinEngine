/** @file @brief Private shared capture and geometry helpers; no SDL required. */
#ifndef PUI_CORE_INTERNAL_H
#define PUI_CORE_INTERNAL_H
#include "pengui/pengui.h"
#include <math.h>

static inline float pui_clamp(float value, float low, float high) {
    return fmaxf(low, fminf(high, value));
}

bool pui_context_visible(const PUIContext *context, float x, float y);
bool pui_context_acquire(PUIContext *context, PUIId id, PUIRect area);
#endif
