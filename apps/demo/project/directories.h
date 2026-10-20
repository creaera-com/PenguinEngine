/** @file @brief Validate and create a single portable child directory. */
#ifndef PENGUENGINE_PROJECT_DIRECTORIES_H
#define PENGUENGINE_PROJECT_DIRECTORIES_H
#include "project.h"
/** @brief Reject separators, traversal, reserved names and existing entries; return new path. */
bool project_directory_create(const char *parent, const char *name, char *path,
                              size_t path_capacity, char *error, size_t error_capacity);
#endif
