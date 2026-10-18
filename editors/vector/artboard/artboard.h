/** @file @brief Artboard data and undoable creation commands. */
#ifndef PE_VECTOR_ARTBOARD_H
#define PE_VECTOR_ARTBOARD_H
#include "editors/vector/history/history.h"
#include "pengui/pengui.h"
#include <stdio.h>
#define VECTOR_MAX_ARTBOARDS 64
#define VECTOR_MAX_ARTBOARD_SIZE 1000000.0f

typedef struct {
    unsigned id;
    char name[64];
    PUIRect rect; ///< Position and size in document pixels, independent of viewport zoom.
} VectorArtboard;
struct VectorDocument;
/** @brief Change name and dimensions atomically through history. */
bool vector_artboard_update(struct VectorDocument *document, unsigned id, const char *name,
                            float width, float height);
/** @brief Create an artboard through history; rectangle must be finite and at least 1x1 pixel. */
bool vector_artboard_add(struct VectorDocument *document, PUIRect rect);
/** @brief Session codec: write command values, never function or heap pointers. */
bool vector_artboard_command_write(const VectorCommand *command, FILE *file);
/** @brief Reconstruct callbacks and allocate owned data; caller frees via history on success. */
bool vector_artboard_command_read(VectorCommand *command, FILE *file);
#endif
