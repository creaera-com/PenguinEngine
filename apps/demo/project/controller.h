/** @file
 * @brief Shell actions, async native dialogs, project identity and Explorer coordination. */
#ifndef PENGUENGINE_PROJECT_CONTROLLER_H
#define PENGUENGINE_PROJECT_CONTROLLER_H
#include "explorer.h"

typedef struct {
    AppProject project;
    ProjectExplorer explorer;
    char state_file[PROJECT_PATH_CAPACITY];
    Uint32 dialog_event;
    bool dialog_pending;
} ProjectController;

/** @brief Initialize shared UI; restore last project only when remember is true. */
bool project_controller_init(ProjectController *controller, const char *resources, bool remember,
                             char *error, size_t capacity);
void project_controller_destroy(ProjectController *controller);
/** @brief Open an explicit file or create in a folder, then update state and explorer. */
bool project_controller_open(ProjectController *controller, const char *path, bool create,
                             PUIView *view);
/** @brief Start a native chooser, or dispatch Explorer actions. */
bool project_controller_action(ProjectController *controller, SDL_Window *window, PUIView *view,
                               const char *action, const char *widget);
/** @brief Complete dialog results on the main thread; callbacks never touch UI objects. */
bool project_controller_event(ProjectController *controller, PUIView *view, const SDL_Event *event);
#endif
