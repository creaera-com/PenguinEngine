/** @file
 * @brief Project identity and metadata; no editor or widget dependencies. */
#ifndef PENGUENGINE_PROJECT_H
#define PENGUENGINE_PROJECT_H
#include <stdbool.h>
#include <stddef.h>
#define PROJECT_PATH_CAPACITY 4096

typedef struct {
    char file[PROJECT_PATH_CAPACITY], root[PROJECT_PATH_CAPACITY], name[256];
} AppProject;

/** @brief Read and validate project metadata transactionally. */
bool app_project_open(AppProject *project, const char *file, char *error, size_t capacity);
/** @brief Exclusively create project.pengui.yaml in an existing selected folder. */
bool app_project_create(AppProject *project, const char *folder, char *error, size_t capacity);
#endif
