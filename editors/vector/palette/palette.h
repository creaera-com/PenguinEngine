/** @file @brief Pure HSL harmony generation, independent of documents and UI. */
#ifndef VECTOR_PALETTE_H
#define VECTOR_PALETTE_H
#include "pengui/pengui.h"

typedef enum {
    VECTOR_HARMONY_MONO,
    VECTOR_HARMONY_ANALOGOUS,
    VECTOR_HARMONY_TRIAD,
    VECTOR_HARMONY_TETRAD,
    VECTOR_HARMONY_COMPLEMENT,
    VECTOR_HARMONY_SPLIT,
    VECTOR_HARMONY_SQUARE
} VectorHarmony;

/** @brief Generate 2–5 RGBA colors; retain base alpha. Spread is clamped to 5–90 degrees. */
int vector_palette_generate(uint32_t base, VectorHarmony harmony, float spread, uint32_t out[5]);
#endif
