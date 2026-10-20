/** @file @brief Copy and resolve project-owned image files without editor UI dependencies. */
#ifndef PE_VECTOR_IMAGE_ASSETS_H
#define PE_VECTOR_IMAGE_ASSETS_H
#include "state.h"
#include <stddef.h>
/** @brief Find nearest project metadata above a document, falling back to its directory. */
bool vector_image_project_root(const char *document, char *root, size_t capacity);
/** @brief Validate a relative reference restricted to assets/images and a single basename. */
bool vector_image_relative_valid(const char *relative);
/** @brief Exclusively copy to assets/images using a unique name; never overwrite source files. */
bool vector_image_copy(const char *source, const char *root, char *target, size_t capacity);
/** @brief Ensure a referenced image exists under the destination project for document Save As. */
bool vector_image_export(const VectorImage *image, const char *root, char *relative,
                         size_t capacity);
#endif
