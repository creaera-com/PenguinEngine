/** @file
 * @brief Application configuration, independent of widget YAML. */
#ifndef PE_EDITOR_CONFIG_H
#define PE_EDITOR_CONFIG_H
#include <stdbool.h>
#include <stddef.h>

typedef struct {
    char startup_editor[32];
} EditorConfig;

/** @brief Read a strict configuration with the required startup_editor scalar. */
bool editor_config_load(EditorConfig *config, const char *path, char *error, size_t capacity);
#endif
