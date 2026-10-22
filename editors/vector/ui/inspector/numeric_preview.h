/** @file @brief Disposable inspector scrub preview; never changes the live document/history. */
#ifndef PE_VECTOR_NUMERIC_PREVIEW_H
#define PE_VECTOR_NUMERIC_PREVIEW_H
#include "editors/vector/document/document.h"
struct VectorInspector;

/** @brief Owns one temporary document workspace only while a numeric gesture is in progress. */
typedef struct {
    VectorDocuments *documents;
    unsigned source_id, object_id, artboard_id;
    int cursor;
    char field[64], text[256];
    bool valid;
} VectorNumericPreview;

/** @brief Evaluate the current field using existing inspector commands against an isolated copy. */
void vector_numeric_preview_sync(VectorNumericPreview *preview,
                                 const struct VectorInspector *inspector, VectorDocument *source,
                                 PUIView *view);
/** @brief Borrow the preview when it belongs to source, otherwise return source unchanged. */
VectorDocument *vector_numeric_preview_document(VectorNumericPreview *preview,
                                                VectorDocument *source);
/** @brief Release transient workspace; safe repeatedly and on editor unload. */
void vector_numeric_preview_destroy(VectorNumericPreview *preview);
#endif
