#ifndef PENGUIN_MAP_EDITOR_H
#define PENGUIN_MAP_EDITOR_H

#include "penguin/render.h"

typedef enum { BRUSH, ERASE, RECTANGLE, FILL, SELECT } Tool;

typedef struct {
    PeScene scene, runtime;
    PeHistory history;
    PeAssets assets;
} MapPayload;

#endif
