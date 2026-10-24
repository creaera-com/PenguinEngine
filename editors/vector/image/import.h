/** @file
 * @brief Import validated project copies through the common editor adapter. */
#ifndef PE_VECTOR_IMAGE_IMPORT_H
#define PE_VECTOR_IMAGE_IMPORT_H
#include "editor_host/editor.h"
/** @brief Place into the captured document/artboard, centered at natural size or fitted smaller. */
bool vector_image_import(void *instance, PUIView *view, unsigned document, unsigned artboard,
                         const char *source, const char *project, char *error, size_t capacity);
#endif
