#ifndef PENGUIN_RENDER_H
#define PENGUIN_RENDER_H
#include "penguin/engine.h"
#include <SDL3/SDL.h>
typedef struct {
    SDL_Texture *tileset;
    int width, height;
    char path[PE_PATH];
    int source_tile;
} PeAssets;
bool pe_assets_load(PeAssets *assets, SDL_Renderer *renderer, const char *root,
                    const PeScene *scene, char *error, size_t size);
void pe_assets_free(PeAssets *assets);
void pe_render_scene(SDL_Renderer *renderer, const PeScene *scene, const PeAssets *assets,
                     const PeCamera *camera, SDL_FRect viewport, bool grid, bool collision);
#endif
