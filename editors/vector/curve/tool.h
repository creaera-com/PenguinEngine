/** @file
 * @brief Native pen and node tool lifecycle, separated from curve commands. */
#ifndef PE_VECTOR_CURVE_TOOL_H
#define PE_VECTOR_CURVE_TOOL_H
#include "curve.h"
/** @brief Discard incomplete creation/editing and release capture without changing history. */
void vector_curve_tool_cancel(VectorDocument *document);
/** @brief Handle vector.tool_pen and vector.tool_nodes; tools are mutually exclusive. */
bool vector_curve_tool_action(VectorDocument *document, PUIView *view, const char *action);
/** @brief Click/drag creation and anchor/handle editing. Enter/right/double click finishes open;
 * clicking the first node closes; Escape/focus loss cancels. Alt moves handles independently. */
bool vector_curve_tool_event(VectorDocument *document, PUIView *view, const SDL_Event *event);
/** @brief Return caller-owned live geometry, resolving linked appearance without changing the
 * model. */
VectorShape vector_curve_display_shape(const VectorDocument *document, const VectorShape *shape);
#endif
