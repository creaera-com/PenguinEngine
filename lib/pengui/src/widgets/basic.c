/** @file @brief Small SDL-free controls: disclosure behavior. */
#include "core/internal.h"
#include <string.h>

bool ui_disclosure(UiContext *c, UiId id, UiRect r, bool *open, bool enabled) {
    if (!ui_button(c, id, r, enabled)) {
        return false;
    }
    *open = !*open;
    return true;
}
