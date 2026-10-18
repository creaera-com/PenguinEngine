/** @file @brief Plain per-document viewport state; no renderer or event processing. */
#ifndef PE_VECTOR_VIEWPORT_H
#define PE_VECTOR_VIEWPORT_H
#include "pengui/pengui.h"

typedef struct {
    PUIRect bounds;
    float x, y, zoom;
    float mouse_x, mouse_y;
    bool panning;
} VectorViewport;
#endif
