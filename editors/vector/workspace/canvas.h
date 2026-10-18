/** @file @brief Vector canvas rendering and viewport interaction. */
#ifndef PE_VECTOR_CANVAS_H
#define PE_VECTOR_CANVAS_H
#include "pengui/pengui.h"
#include "viewport.h"
struct VectorDocument;

/** @brief Build a generic canvas node in C; no document YAML is needed. */
PUILayout *vector_canvas_layout(char *error, size_t capacity);
void vector_canvas_draw(struct VectorDocument *document, SDL_Renderer *renderer, PUIRect area);
bool vector_canvas_event(struct VectorDocument *document, PUIView *view, const SDL_Event *event);
/** @brief Center the selected artboard without modifying the document history. */
void vector_canvas_center(struct VectorDocument *document);
#endif
