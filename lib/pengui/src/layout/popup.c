/** @file @brief SDL-free placement for anchored popups. */
#include "pengui/pengui.h"
#include <math.h>

PUIRect pui_popup_rect(PUIRect bounds, PUIRect anchor, float width, float height,
                       PUIPopupPlacement placement) {
    float w = fmaxf(0, fminf(width, bounds.w));
    float h = fmaxf(0, fminf(height, bounds.h));
    float x = anchor.x, y = anchor.y + anchor.h + 3;
    if (placement == PUI_POPUP_RIGHT) {
        x = anchor.x + anchor.w + 3;
        y = anchor.y;
        if (x + w > bounds.x + bounds.w) {
            x = anchor.x - w - 3;
        }
    } else if (y + h > bounds.y + bounds.h) {
        y = anchor.y - h - 3;
    }
    x = fmaxf(bounds.x, fminf(x, bounds.x + bounds.w - w));
    y = fmaxf(bounds.y, fminf(y, bounds.y + bounds.h - h));
    return (PUIRect){x, y, w, h};
}
