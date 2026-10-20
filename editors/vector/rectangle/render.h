/** @file @brief Rectangle rendering uses parent transforms and artboard clipping. */
#ifndef PE_VECTOR_RECTANGLE_RENDER_H
#define PE_VECTOR_RECTANGLE_RENDER_H
#include "editors/vector/document/document.h"
#include "editors/vector/image/render.h"
#include "editors/vector/text/render.h"
void vector_rectangles_draw(VectorDocument *document, const VectorArtboard *parent,
                            SDL_Renderer *renderer, SDL_FRect board, VectorTextRenderer *text,
                            VectorImageRenderer *images);
#endif
