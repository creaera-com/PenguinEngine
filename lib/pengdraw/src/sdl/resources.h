/** @file
 * @brief Private accounting for pengDraw-owned texture handles and fallback attempts. */
#ifndef PDRAW_SDL_RESOURCES_H
#define PDRAW_SDL_RESOURCES_H
#include "pengdraw/pengdraw.h"

typedef enum {
    PDRAW_TEXTURE_ARTWORK,
    PDRAW_TEXTURE_SHADOW,
    PDRAW_TEXTURE_MASK,
    PDRAW_TEXTURE_GRADIENT,
    PDRAW_TEXTURE_AUXILIARY
} PDrawTextureClass;

SDL_Texture *pdraw_create_texture(SDL_Renderer *, SDL_PixelFormat, SDL_TextureAccess, int width,
                                  int height, PDrawTextureClass);
SDL_Texture *pdraw_create_texture_from_surface(SDL_Renderer *, SDL_Surface *, PDrawTextureClass);
void pdraw_record_fallback(SDL_Renderer *, bool shadow);
#endif
