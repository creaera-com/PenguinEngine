/** @file @brief Portable exclusive file creation with UTF-8 filenames. */
#ifndef PENGUENGINE_PLATFORM_FILES_H
#define PENGUENGINE_PLATFORM_FILES_H
#include <stdbool.h>
#include <stddef.h>
/** @brief Create/write/close a new file atomically with respect to existence; never overwrite. */
bool editor_file_create_exclusive(const char *path, const void *data, size_t length);
#endif
