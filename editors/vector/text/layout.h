/** @file @brief Renderer-independent UTF-8 line breaking and column flow. */
#ifndef PE_VECTOR_TEXT_LAYOUT_H
#define PE_VECTOR_TEXT_LAYOUT_H
#include "state.h"
#include <stddef.h>

/** @brief One codepoint's position and byte identity; coordinates are frame-local pixels. */
typedef struct {
    unsigned code;
    size_t byte, end;
    float x, y, advance;
} VectorTextGlyph;

/** @brief Bounded text flow result; overflow marks content that cannot fit the frame. */
typedef struct {
    VectorTextGlyph glyphs[VECTOR_TEXT_CAPACITY];
    int count;
    bool overflow;
    size_t visible_end;
    float end_x, end_y;
} VectorTextLayout;

typedef float (*VectorTextAdvance)(void *context, unsigned code, unsigned next, float size);
/** @brief Wrap words, split long words, align lines and flow into up to sixteen columns. */
void vector_text_layout(const VectorText *text, float width, float height,
                        VectorTextAdvance advance, void *context, VectorTextLayout *out);
/** @brief Decode one validated UTF-8 codepoint and advance a byte offset. */
unsigned vector_text_codepoint(const char *text, size_t *offset);
#endif
