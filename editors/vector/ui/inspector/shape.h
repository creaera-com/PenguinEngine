/** @file @brief Optional rectangle inspector and field-to-model bindings. */
#ifndef PE_VECTOR_RECTANGLE_INSPECTOR_H
#define PE_VECTOR_RECTANGLE_INSPECTOR_H
#include "editor_host/editor.h"
#include "editors/vector/document/document.h"
#include "gradient.h"
#include "text.h"

typedef struct {
    PUILayout *layout, *polygon_layout, *image_layout, *gradient_layout, *corners_layout;
    VectorTextInspector text;
    VectorGradientInspector gradient;
    unsigned document_id, rectangle_id, dirty;
    VectorRectangle presented;
    bool mounted, image_mounted, angle_preview, polygon_mounted, corners_mounted;
    float displayed_angle;
} VectorRectangleInspector;

/** @brief Cancel an in-progress slider before a tool or history action changes context. */
void vector_rectangle_inspector_cancel_color(VectorRectangleInspector *inspector,
                                             VectorDocuments *documents, PUIView *view);
bool vector_rectangle_inspector_action(VectorRectangleInspector *inspector,
                                       VectorDocuments *documents, PUIView *view,
                                       const char *action);
bool vector_rectangle_inspector_sync(VectorRectangleInspector *inspector, VectorDocument *document,
                                     PUIView *view, PUIContext *input, char *error,
                                     size_t capacity);
#endif
