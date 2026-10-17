/** @file @brief SDL presentation of generic YAML widgets. */
#ifndef PENGUI_VIEW_H
#define PENGUI_VIEW_H
#include "pengui/layout.h"
#include "pengui/ui_sdl.h"
/** @brief Application callback. Both strings are borrowed for the duration of the call. */
typedef void (*UiAction)(void *context, const char *action, const char *widget);
/** @brief Opaque view owns widget state and cached textures, borrows its layout and renderer. */
typedef struct UiView UiView;
/** @brief Create one independent view; layout/renderer must outlive it. NULL on failure. */
UiView *ui_view_new(const UiLayout *layout, SDL_Renderer *renderer);
/** @brief Destroy before the renderer/layout; NULL is accepted. */
void ui_view_free(UiView *view);
/** @brief Apply the seven theme colors from YAML to the existing renderer theme. */
void ui_view_apply_theme(const UiLayout *layout);
/** @brief Feed SDL input on the main thread; true when a text edit consumed the event. */
bool ui_view_event(UiView *view, UiContext *ui, SDL_Window *window, const SDL_Event *event);
/** @brief Draw a frame; callbacks may change app state but must not free the active view/layout. */
void ui_view_draw(UiView *view, UiContext *ui, UiRect area, UiAction action, void *context);
/** @brief Read an input value by YAML id; NULL for unknown/non-input widgets. Borrowed until free.
 */
const char *ui_view_text(const UiView *view, const char *id);
/** @brief Set input/label text without modifying YAML; false for wrong kinds or excessive text. */
bool ui_view_set_text(UiView *view, const char *id, const char *text);
/** @brief Load a bounded PNG image as a renderer-owned texture; caller must destroy it. */
SDL_Texture *ui_image_load(SDL_Renderer *renderer, const char *path);
#endif
