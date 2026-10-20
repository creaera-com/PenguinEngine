/** @file @brief Versioned, endian-independent .pengvec document files (no native struct dumps). */
#ifndef PENGUENGINE_VECTOR_DOCUMENT_FILE_H
#define PENGUENGINE_VECTOR_DOCUMENT_FILE_H
#include "editors/vector/document/document.h"
/** @brief Validate/load into a fresh initialized document; failure leaves it unchanged. */
bool vector_document_file_load(VectorDocument *document, const char *path, char *error,
                               size_t capacity);
/** @brief Save through a sibling temporary file and rename; update filename/title only on success.
 */
bool vector_document_file_save(VectorDocument *document, const char *path, char *error,
                               size_t capacity);
#endif
