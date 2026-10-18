/** @file @brief Artboard data and undoable creation commands. */
#ifndef PE_VECTOR_ARTBOARD_H
#define PE_VECTOR_ARTBOARD_H
#include "pengui/pengui.h"
#define VECTOR_MAX_ARTBOARDS 64

typedef struct {
    unsigned id;
    PUIRect rect; ///< Position and size in document pixels, independent of viewport zoom.
} VectorArtboard;
struct VectorDocument;
/** @brief Create a 512x512 artboard through the document history. */
bool vector_artboard_add(struct VectorDocument *document);
#endif
