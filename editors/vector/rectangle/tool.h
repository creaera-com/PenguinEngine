/** @file @brief Rectangle drawing tool; final geometry enters history only on release. */
#ifndef PE_VECTOR_RECTANGLE_TOOL_H
#define PE_VECTOR_RECTANGLE_TOOL_H
#include "editors/vector/document/document.h"
void vector_rectangle_tool_cancel(VectorDocument *document);
/** @brief Toggle the tool and cancel other gestures; no document content is created yet. */
bool vector_rectangle_tool_action(VectorDocument *document, PUIView *view, const char *action);
bool vector_rectangle_tool_event(VectorDocument *document, PUIView *view, const SDL_Event *event);
PUIRect vector_rectangle_tool_rect(const VectorRectangleTool *tool);
#endif
