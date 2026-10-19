/** @file @brief Pointer-free UTF-8 text and paragraph style values. */
#ifndef PE_VECTOR_TEXT_STATE_H
#define PE_VECTOR_TEXT_STATE_H
#include <stdbool.h>
#define VECTOR_TEXT_CAPACITY 2048

/** @brief Paragraph alignment. Justification excludes the final line of a paragraph. */
typedef enum {
    VECTOR_TEXT_LEFT,
    VECTOR_TEXT_CENTER,
    VECTOR_TEXT_RIGHT,
    VECTOR_TEXT_JUSTIFY
} VectorTextAlignment;

/** @brief One uniformly styled, multi-column text frame; empty font path uses bundled font. */
typedef struct {
    unsigned enabled, face, bold, italic, alignment, columns;
    char content[VECTOR_TEXT_CAPACITY], font[512];
    float size, line_height, letter_spacing, column_gap;
} VectorText;

/** @brief Validate style bounds, string termination and UTF-8 including newlines. */
bool vector_text_valid(const VectorText *text);
#endif
