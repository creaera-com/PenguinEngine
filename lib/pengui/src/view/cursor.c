/** @file @brief Resolve widget cursor requests and own native system cursors per view. */
#include "internal.h"

void pui_view_request_cursor(PUIView *view, SDL_SystemCursor cursor, int priority) {
    if (priority >= view->cursor_priority) {
        view->cursor = cursor;
        view->cursor_priority = priority;
    }
}

SDL_SystemCursor pui_view_cursor(const PUIView *view) {
    return view ? view->cursor : SDL_SYSTEM_CURSOR_DEFAULT;
}

void pui_view_apply_cursor(PUIView *view, const PUIContext *pui) {
    if (!view->pointer_inside && !pui->active) {
        view->cursor = SDL_SYSTEM_CURSOR_DEFAULT;
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
    /* Backends without native cursors (e.g. dummy) may return NULL. */
    if (cursor && SDL_GetCursor() != cursor) {
        SDL_SetCursor(cursor);
    }
}

void pui_view_free_cursors(PUIView *view) {
    for (int i = 0; i < SDL_SYSTEM_CURSOR_COUNT; i++) {
        if (view->cursors[i]) {
            if (SDL_GetCursor() == view->cursors[i]) {
                SDL_SetCursor(SDL_GetDefaultCursor());
            }
            SDL_DestroyCursor(view->cursors[i]);
        }
    }
}
