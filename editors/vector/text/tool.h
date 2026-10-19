/** @file @brief Native UTF-8 typing, selection and clipboard for canvas text frames. */
#ifndef PE_VECTOR_TEXT_TOOL_H
#define PE_VECTOR_TEXT_TOOL_H
#include "render.h"
/** @brief Start an independent typing history group and select the existing content. */
void vector_text_tool_begin(VectorDocument *document, unsigned id, SDL_Window *window);
/** @brief End the typing group; edits are already in model/history. */
void vector_text_tool_end(VectorDocument *document);
/** @brief Route native typing before object shortcuts, preserving UI-focused input handling. */
bool vector_text_tool_event(VectorDocument *document, VectorTextRenderer *renderer, PUIView *view,
                            const SDL_Event *event);
#endif
