/** @file @brief Artboard data and undoable creation, geometry and property commands. */
#ifndef PE_VECTOR_ARTBOARD_H
#define PE_VECTOR_ARTBOARD_H
#include "editors/vector/history/history.h"
#include "editors/vector/object/state.h"
#include "pengui/pengui.h"
#include <stdio.h>
#define VECTOR_MAX_ARTBOARDS 64
#define VECTOR_MAX_ARTBOARD_SIZE 1000000.0f

typedef struct {
    unsigned id;
    unsigned flags; ///< Common visibility and lock flags; zero means visible and editable.
    char name[64];
    PUIRect rect; ///< Position and size in document pixels, independent of viewport zoom.
} VectorArtboard;
struct VectorDocument;
/** @brief Find an artboard by its stable ID. */
VectorArtboard *vector_artboard_find(struct VectorDocument *document, unsigned id);
/** @brief Undoable visibility/lock state; remains editable while the object is locked. */
bool vector_artboard_set_state(struct VectorDocument *document, unsigned id, unsigned flags);
/** @brief Atomically change position and dimensions through a single history command. */
bool vector_artboard_set_rect(struct VectorDocument *document, unsigned id, PUIRect rect);
/** @brief Move an artboard through a single undoable position change. */
bool vector_artboard_move(struct VectorDocument *document, unsigned id, float x, float y);
/** @brief Change name and dimensions atomically through history. */
bool vector_artboard_update(struct VectorDocument *document, unsigned id, const char *name,
                            float width, float height);
/** @brief Create an artboard through history; rectangle must be finite and at least 1x1 pixel. */
bool vector_artboard_add(struct VectorDocument *document, PUIRect rect);
/** @brief Session codec: write command values, never function or heap pointers. */
bool vector_artboard_command_write(const VectorCommand *command, FILE *file);
/** @brief Reconstruct callbacks and allocate owned data; caller frees via history on success. */
bool vector_artboard_command_read(VectorCommand *command, FILE *file, unsigned kind);
#endif
