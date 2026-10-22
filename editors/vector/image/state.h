/** @file
 * @brief Image reference values, independent of decoded pixels and GPU resources. */
#ifndef PE_VECTOR_IMAGE_STATE_H
#define PE_VECTOR_IMAGE_STATE_H
#include <stdbool.h>

typedef struct {
    char source[1024];      ///< Resolved project asset path at runtime; relative in document files.
    unsigned width, height; ///< Original pixel dimensions; zero with empty source means no image.
    unsigned flip_x, flip_y;
} VectorImage;

/** @brief Validate a terminated reference and bounded original pixel dimensions. */
bool vector_image_valid(const VectorImage *image);
#endif
