/** @file @brief Common object inspector and field-to-model bindings. */
#ifndef PE_VECTOR_OBJECT_INSPECTOR_H
#define PE_VECTOR_OBJECT_INSPECTOR_H
#include "../appearance/fill/gradient.h"
#include "../text/text.h"
#include "editor_host/editor.h"
#include "editors/vector/document/document.h"

typedef struct {
    PUILayout *layout, *polygon_layout, *image_layout, *gradient_layout, *corners_layout,
        *ellipse_layout;
    VectorTextInspector text;
    VectorGradientInspector gradient;
    unsigned document_id, object_id, dirty;
    VectorShape presented;
    bool mounted, image_mounted, angle_preview, polygon_mounted, corners_mounted, ellipse_mounted;
    float displayed_angle;
} VectorObjectInspector;

/** @brief Cancel an in-progress slider before a tool or history action changes context. */
void vector_object_inspector_cancel_color(VectorObjectInspector *inspector,
                                          VectorDocuments *documents, PUIView *view);
/** @brief Route object fields and appearance sections to their domain commands. */
bool vector_object_inspector_action(VectorObjectInspector *inspector, VectorDocuments *documents,
                                    PUIView *view, const char *action);
/** @brief Mount optional sections and synchronize committed values between frames. */
bool vector_object_inspector_sync(VectorObjectInspector *inspector, VectorDocument *document,
                                  PUIView *view, PUIContext *input, char *error, size_t capacity);
#endif
