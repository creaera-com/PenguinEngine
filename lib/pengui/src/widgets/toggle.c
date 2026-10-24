/** @file @brief SDL-free checked state and mutually exclusive selection. */
#include "pengui/pengui.h"

bool pui_toggle(PUIContext *context, PUIId id, PUIRect rect, bool *checked, bool enabled) {
    if (!checked || !pui_button(context, id, rect, enabled)) {
        return false;
    }
    *checked = !*checked;
    return true;
}

bool pui_radio(PUIContext *context, PUIId id, PUIRect rect, PUIId *selected, bool enabled) {
    if (!selected || !pui_button(context, id, rect, enabled) || *selected == id) {
        return false;
    }
    *selected = id;
    return true;
}
