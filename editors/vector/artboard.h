/** @file @brief Artboard data and undoable creation commands. */
#ifndef PE_VECTOR_ARTBOARD_H
#define PE_VECTOR_ARTBOARD_H
#include "history.h"
#include "pengui/pengui.h"
#include <stdio.h>
#define VECTOR_MAX_ARTBOARDS 64

typedef struct {
    unsigned id;
    PUIRect rect; ///< Position and size in document pixels, independent of viewport zoom.
} VectorArtboard;
struct VectorDocument;
/** @brief Create an artboard through history; rectangle must be finite and at least 1x1 pixel. */
bool vector_artboard_add(struct VectorDocument *document, PUIRect rect);
/** @brief Session codec: write command values, never function or heap pointers. */
bool vector_artboard_command_write(const VectorCommand *command, FILE *file);
/** @brief Reconstruct callbacks and allocate owned data; caller frees via history on success. */
bool vector_artboard_command_read(VectorCommand *command, FILE *file);
#endif
