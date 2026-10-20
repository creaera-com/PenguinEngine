/** @file @brief Value-only raster export scopes and document-space bounds. */
#ifndef PE_VECTOR_EXPORT_PLAN_H
#define PE_VECTOR_EXPORT_PLAN_H
#include "editors/vector/document/document.h"

/** @brief All artboards produce individual files; selection may span artboards. */
typedef enum {
    VECTOR_EXPORT_ALL,
    VECTOR_EXPORT_ARTBOARD,
    VECTOR_EXPORT_SELECTION
} VectorExportScope;

typedef struct {
    unsigned board;
    PUIRect bounds;
} VectorExportItem;

typedef struct {
    VectorExportScope scope;
    unsigned count;
    VectorExportItem items[VECTOR_MAX_ARTBOARDS];
} VectorExportPlan;

/** @brief Plan integer-pixel bounds, including visible strokes/shadows/repeats, without edits.
 * Each output is limited to 8192 per axis and 16 million pixels. Hidden layers are excluded.
 */
bool vector_export_plan(const VectorDocument *document, VectorExportScope scope,
                        VectorExportPlan *plan, char *error, size_t capacity);
#endif
