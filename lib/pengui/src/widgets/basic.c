/** @file @brief Small SDL-free controls: disclosure behavior. */
#include "core/internal.h"
#include <string.h>

bool pui_disclosure(PUIContext *c, PUIId id, PUIRect r, bool *open, bool enabled) {
    if (!pui_button(c, id, r, enabled)) {
        return false;
    }
    *open = !*open;
    return true;
}
