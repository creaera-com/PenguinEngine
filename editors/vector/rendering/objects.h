/** @file @brief Editor scene composition: resolved objects, layers, tools and artboard clipping. */
#ifndef PE_VECTOR_OBJECT_RENDER_H
#define PE_VECTOR_OBJECT_RENDER_H
#include "editors/vector/document/document.h"
#include "editors/vector/image/render.h"
#include "editors/vector/text/render.h"
void vector_objects_draw(VectorDocument *document, const VectorArtboard *parent,
                         SDL_Renderer *renderer, SDL_FRect board, VectorTextRenderer *text,
                         VectorImageRenderer *images);
#endif
