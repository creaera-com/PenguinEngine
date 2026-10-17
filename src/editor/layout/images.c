#include "editor/layout/images.h"
#include "penguin/image.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct PeUiImageEntry {
    char path[2048];
    SDL_Texture *texture;
};

void pe_ui_images_free(PeUiImages *images) {
    for (int i = 0; i < images->count; i++) {
        SDL_DestroyTexture(images->entries[i].texture);
    }
    free(images->entries);
    *images = (PeUiImages){0};
}

SDL_Texture *pe_ui_images_get(PeUiImages *images, SDL_Renderer *renderer, const PeUiLayout *layout,
                              const char *path) {
    if (!layout || !path || !*path) {
        return NULL;
    }
    char directory[1024], resolved[2048];
    snprintf(directory, sizeof directory, "%s", layout->path);
    char *slash = strrchr(directory, '/'), *backslash = strrchr(directory, '\\');
    if (backslash && (!slash || backslash > slash)) {
        slash = backslash;
    }
    if (slash) {
        slash[1] = 0;
    } else {
        directory[0] = 0;
    }
    bool absolute = *path == '/' || *path == '\\' || (path[0] && path[1] == ':');
    if (snprintf(resolved, sizeof resolved, "%s%s", absolute ? "" : directory, path) >=
        (int)sizeof resolved) {
        return NULL;
    }
    for (int i = 0; i < images->count; i++) {
        if (!strcmp(images->entries[i].path, resolved)) {
            return images->entries[i].texture;
        }
    }
    if (images->count == 64) {
        return NULL;
    }
    if (!images->entries) {
        images->entries = calloc(64, sizeof *images->entries);
        if (!images->entries) {
            return NULL;
        }
    }
    PeUiImageEntry *entry = &images->entries[images->count++];
    snprintf(entry->path, sizeof entry->path, "%s", resolved);
    char error[2300];
    entry->texture = pe_image_load(renderer, resolved, error, sizeof error);
    if (!entry->texture) {
        SDL_Log("%s", error);
    }
    return entry->texture;
}
