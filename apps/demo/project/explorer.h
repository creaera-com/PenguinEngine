/** @file
 * @brief Shared project file-tree binding; lazy directory scans run between frames. */
#ifndef PENGUENGINE_PROJECT_EXPLORER_H
#define PENGUENGINE_PROJECT_EXPLORER_H
#include "pengui/pengui.h"
#include "project.h"
typedef struct ProjectEntry ProjectEntry;

typedef struct {
    ProjectEntry *entries;
    int count, allocated;
    char root[PROJECT_PATH_CAPACITY];
    bool dirty, reset;
    char filter[256];
    int search_cursor;
    PUILayout *layout;
    char pending_parent[PROJECT_PATH_CAPACITY], pending_name[256];
    bool create_pending;
} ProjectExplorer;

/** @brief Load the common explorer UI; caller owns the binding until destroy. */
bool project_explorer_init(ProjectExplorer *explorer, const char *resources, char *error,
                           size_t capacity);
void project_explorer_destroy(ProjectExplorer *explorer);
/** @brief Replace tree identity after a successful project change. */
void project_explorer_set(ProjectExplorer *explorer, const AppProject *project);
/** @brief Borrow the filesystem path of a file row; NULL for folders or invalid IDs. */
const char *project_explorer_file(const ProjectExplorer *explorer, const char *row);
/** @brief Handle row selection and directory expansion; no filesystem work during draw. */
bool project_explorer_action(ProjectExplorer *explorer, PUIView *view, const char *action,
                             const char *widget);
/** @brief Mount the app-owned explorer slot after editor switching and publish rows. */
bool project_explorer_commit(ProjectExplorer *explorer, PUIView *view, PUIContext *input,
                             char *error, size_t capacity);
#endif
