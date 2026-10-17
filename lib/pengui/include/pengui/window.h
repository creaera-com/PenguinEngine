/** @file @brief Optional borderless SDL window controls, independent of application actions. */
#ifndef PENGUI_WINDOW_H
#define PENGUI_WINDOW_H
#include "pengui/view.h"

/** @brief Stable callback context. Must outlive the attached window or be detached first. */
typedef struct {
    SDL_Window *window;  ///< Borrowed window.
    const UiView *view;  ///< Borrowed view; update after YAML reload.
    const UiContext *ui; ///< Borrowed interaction context.
} UiWindow;

/** @brief Attach hit testing and remove native borders; retain native borders if unsupported. */
bool ui_window_attach(UiWindow *chrome, SDL_Window *window, const UiView *view,
                      const UiContext *ui);
/** @brief Remove callback before releasing its borrowed view/context. */
void ui_window_detach(UiWindow *chrome);
/** @brief Compute resize/drag regions using the last drawn YAML view; coordinates are window-local.
 */
SDL_HitTestResult ui_window_hit_test(const UiWindow *chrome, int x, int y);
/** @brief Handle window.minimize/maximize/close. Close queues a close request for the application.
 */
bool ui_window_action(SDL_Window *window, const char *action);
#endif
