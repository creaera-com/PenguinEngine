/** @file
 * @brief Persistent last-project location, separate from temporary editor checkpoints. */
#ifndef PENGUENGINE_PROJECT_STATE_H
#define PENGUENGINE_PROJECT_STATE_H
#include "project.h"
/** @brief Resolve/create platform user-state directory (Linux XDG_STATE_HOME/penguengine). */
bool app_project_state_path(char *path, size_t capacity);
/** @brief Store/retrieve the last absolute project filename; writes use replace-on-success. */
bool app_project_state_save(const char *state_file, const char *project_file);
bool app_project_state_read(const char *state_file, char *project_file, size_t capacity);
#endif
