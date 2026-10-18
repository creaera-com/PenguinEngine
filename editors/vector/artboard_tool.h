/** @file @brief Transient mouse gesture; only a completed drag creates a history command. */
#ifndef PE_VECTOR_ARTBOARD_TOOL_H
#define PE_VECTOR_ARTBOARD_TOOL_H
#include "pengui/pengui.h"
struct VectorDocument;

typedef struct {
    bool enabled, dragging;
    float start_x, start_y, end_x, end_y; ///< Document pixels, independent of zoom and pan.
} VectorArtboardTool;

/** @brief Arm a single artboard gesture; Escape or focus loss cancels without editing. */
void vector_artboard_tool_arm(struct VectorDocument *document);
/** @brief Release mouse capture and discard the preview without changing history. */
void vector_artboard_tool_cancel(struct VectorDocument *document);
/** @brief Handle left-button drawing, including release outside the canvas. */
bool vector_artboard_tool_event(struct VectorDocument *document, PUIView *view,
                                const SDL_Event *event);
/** @brief Normalized preview rectangle in whole document pixels. */
PUIRect vector_artboard_tool_rect(const VectorArtboardTool *tool);
#endif
