/** @file @brief Resolve widget cursor requests and own native system cursors per view. */
#include "internal.h"
#include <math.h>

void pui_view_request_cursor(PUIView *view, SDL_SystemCursor cursor, int priority) {
    if (view && cursor >= SDL_SYSTEM_CURSOR_DEFAULT && cursor < SDL_SYSTEM_CURSOR_COUNT &&
        priority >= 1 && priority >= view->cursor_priority) {
        view->rotation_cursor_requested = false;
        view->cursor = cursor;
        view->cursor_priority = priority;
    }
}

void pui_view_request_rotation_cursor(PUIView *view, int priority) {
    if (view && priority >= 1 && priority >= view->cursor_priority) {
        pui_view_request_cursor(view, SDL_SYSTEM_CURSOR_CROSSHAIR, priority);
        view->rotation_cursor_requested = true;
    }
}

bool pui_view_rotation_cursor(const PUIView *view) {
    return view && view->rotation_cursor_requested;
}

/* A clockwise arrow with a white outline stays readable on both light artwork
 * and dark workspace backgrounds. The hotspot is the center of the rotation ring. */
static SDL_Cursor *create_rotation_cursor(void) {
    bool ink[32][32] = {{false}};
    for (int y = 1; y < 31; y++) {
        for (int x = 1; x < 31; x++) {
            float dx = x - 16.f, dy = y - 16.f;
            float angle = atan2f(dy, dx) * 57.295779513f;
            bool arc = fabsf(hypotf(dx, dy) - 9) <= 1.25f && angle >= -140 && angle <= 90;
            bool head = x >= 9 && x <= 17 && fabsf(y - 25.f) <= (x - 9) * .625f;
            ink[y][x] = arc || head;
        }
    }
    Uint8 data[128] = {0}, mask[128] = {0};
    for (int y = 0; y < 32; y++) {
        for (int x = 0; x < 32; x++) {
            bool outline = false;
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    int xx = x + dx, yy = y + dy;
                    if (xx >= 0 && xx < 32 && yy >= 0 && yy < 32 && ink[yy][xx]) {
                        outline = true;
                    }
                }
            }
            Uint8 bit = (Uint8)(0x80u >> (x % 8));
            if (ink[y][x]) {
                data[y * 4 + x / 8] |= bit;
            }
            if (outline) {
                mask[y * 4 + x / 8] |= bit;
            }
        }
    }
    return SDL_CreateCursor(data, mask, 32, 32, 16, 16);
}

const PUIContext *pui_view_canvas_input(const PUIView *view) {
    return view && !view->popup_count && !view->radial_open && view->radial_pending < 0
               ? view->canvas_input
               : NULL;
}

SDL_SystemCursor pui_view_cursor(const PUIView *view) {
    return view ? view->cursor : SDL_SYSTEM_CURSOR_DEFAULT;
}

void pui_view_apply_cursor(PUIView *view, const PUIContext *pui) {
    if (!view->pointer_inside && !pui->active && view->cursor_priority < 2) {
        view->cursor = SDL_SYSTEM_CURSOR_DEFAULT;
        view->rotation_cursor_requested = false;
    }
    SDL_Window *mouse_window = SDL_GetMouseFocus();
    if (mouse_window && mouse_window != view->window) {
        return;
    }
    SDL_Cursor *cursor = SDL_GetDefaultCursor();
    if (view->cursor != SDL_SYSTEM_CURSOR_DEFAULT) {
        int index = (int)view->cursor;
        if (!view->cursor_attempted[index]) {
            view->cursor_attempted[index] = true;
            view->cursors[index] = SDL_CreateSystemCursor(view->cursor);
        }
        if (view->cursors[index]) {
            cursor = view->cursors[index];
        }
    }
    if (view->rotation_cursor_requested) {
        if (!view->rotation_cursor_attempted) {
            view->rotation_cursor_attempted = true;
            view->rotation_cursor = create_rotation_cursor();
        }
        if (view->rotation_cursor) {
            cursor = view->rotation_cursor;
        }
    }
    /* Backends without native cursors (e.g. dummy) may return NULL. */
    if (cursor && SDL_GetCursor() != cursor) {
        SDL_SetCursor(cursor);
    }
}

void pui_view_free_cursors(PUIView *view) {
    if (view->rotation_cursor) {
        if (SDL_GetCursor() == view->rotation_cursor) {
            SDL_SetCursor(SDL_GetDefaultCursor());
        }
        SDL_DestroyCursor(view->rotation_cursor);
    }
    for (int i = 0; i < SDL_SYSTEM_CURSOR_COUNT; i++) {
        if (view->cursors[i]) {
            if (SDL_GetCursor() == view->cursors[i]) {
                SDL_SetCursor(SDL_GetDefaultCursor());
            }
            SDL_DestroyCursor(view->cursors[i]);
        }
    }
}
