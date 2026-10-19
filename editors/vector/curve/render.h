/** @file @brief Curve contour drawing and temporary anchor/control-handle overlays. */
#ifndef PE_VECTOR_CURVE_RENDER_H
#define PE_VECTOR_CURVE_RENDER_H
#include "curve.h"
/** @brief Draw closed filled curves or open solid strokes in an already clipped artboard. */
void vector_curve_draw(SDL_Renderer *renderer, const VectorShape *shape, PUIRect rect, float angle,
                       SDL_FRect board, float zoom, uint32_t fill, bool selected);
/** @brief Draw the current pen draft or selected curve's editable anchors and controls. */
void vector_curve_tool_draw(const VectorDocument *document, SDL_Renderer *renderer);
#endif
