/** @file
 * @brief Workspace-only presentation values, independent of artwork and history. */
#ifndef PE_VECTOR_WORKSPACE_STYLE_H
#define PE_VECTOR_WORKSPACE_STYLE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    bool enabled;
    uint32_t color;                     ///< Packed RGBA; multiplied by opacity.
    float opacity;                      ///< Overall opacity, 0..1.
    float offset_x, offset_y, softness; ///< Screen pixels, independent of document zoom.
} VectorWorkspaceStyle;

/** @brief Subtle default artboard shadow; never exported or stored in documents. */
VectorWorkspaceStyle vector_workspace_style_default(void);
/** @brief Strictly read ui/workspace/style.yaml relative to the resource root; atomic on error. */
bool vector_workspace_style_load(VectorWorkspaceStyle *style, const char *resources, char *error,
                                 size_t capacity);
#endif
