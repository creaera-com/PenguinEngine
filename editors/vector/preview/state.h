/** @file
 * @brief Pointer-free, per-document preview region and temporary selection gesture. */
#ifndef PE_VECTOR_PREVIEW_STATE_H
#define PE_VECTOR_PREVIEW_STATE_H
#include "pengui/pengui.h"

typedef struct {
    unsigned board; ///< Stable artboard ID; zero means no committed region.
    PUIRect region; ///< Artboard-local pixels, independent of workspace zoom.
    bool armed, dragging;
    unsigned draft_board;
    float start_x, start_y;
    PUIRect draft;
} VectorPreview;

/** @brief Visible pixel crop and equally sized destination; never stretches the preview. */
typedef struct {
    PUIRect source, destination;
} VectorPreviewWindow;

/** @brief Center small selections; follow canvas center for selections exceeding the viewport. */
bool vector_preview_window(PUIRect selection, PUIRect viewport, float center_x, float center_y,
                           VectorPreviewWindow *result);
/** @brief Validate saved region values, allowing a dormant reference to an undone artboard. */
bool vector_preview_valid(unsigned board, PUIRect region);
#endif
