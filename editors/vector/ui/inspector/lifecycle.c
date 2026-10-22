/** @file @brief Inspector resource ownership, including partially initialized layouts. */
#include "appearance/effects/effects.h"
#include "inspector.h"

static PUILayout *compose_section(const EditorServices *services, PUILayout *base, const char *slot,
                                  const char *file, char *error, size_t capacity) {
    if (!base) {
        return NULL;
    }
    PUILayout *section = editor_load_ui(services, file, error, capacity);
    PUILayout *result = section ? pui_layout_compose(base, slot, section, error, capacity) : NULL;
    pui_layout_free(section);
    pui_layout_free(base);
    return result;
}

bool vector_inspector_init(VectorInspector *inspector, const EditorServices *services,
                           PUIFonts *fonts, VectorTextRenderer *text, char *error,
                           size_t capacity) {
    VectorObjectInspector *object = &inspector->object;
    object->text.fonts = fonts;
    object->text.renderer = text;
    object->text.layout =
        editor_load_ui(services, "editors/vector/ui/inspector/text/text.yaml", error, capacity);
    if (!object->text.layout) {
        goto failed;
    }
    inspector->artboard.layout = editor_load_ui(
        services, "editors/vector/ui/inspector/artboard/artboard.yaml", error, capacity);
    if (!inspector->artboard.layout) {
        goto failed;
    }
    inspector->multi_layout = editor_load_ui(
        services, "editors/vector/ui/inspector/selection/selection.yaml", error, capacity);
    if (!inspector->multi_layout) {
        goto failed;
    }
    object->layout =
        editor_load_ui(services, "editors/vector/ui/inspector/object/object.yaml", error, capacity);
    object->layout =
        compose_section(services, object->layout, "object_fill",
                        "editors/vector/ui/inspector/appearance/fill/fill.yaml", error, capacity);
    object->layout = compose_section(services, object->layout, "object_stroke",
                                     "editors/vector/ui/inspector/appearance/stroke/stroke.yaml",
                                     error, capacity);
    if (!object->layout) {
        goto failed;
    }
    object->layout = vector_effects_layout(services, object->layout, error, capacity);
    if (!object->layout) {
        goto failed;
    }
    object->image_layout =
        editor_load_ui(services, "editors/vector/ui/inspector/image/image.yaml", error, capacity);
    if (!object->image_layout) {
        goto failed;
    }
    object->image_layout = vector_effects_layout(services, object->image_layout, error, capacity);
    if (!object->image_layout) {
        goto failed;
    }
    object->gradient_layout = editor_load_ui(
        services, "editors/vector/ui/inspector/appearance/fill/gradient.yaml", error, capacity);
    if (!object->gradient_layout) {
        goto failed;
    }
    object->corners_layout = editor_load_ui(
        services, "editors/vector/ui/inspector/rectangle/corners.yaml", error, capacity);
    if (!object->corners_layout) {
        goto failed;
    }
    object->polygon_layout = editor_load_ui(
        services, "editors/vector/ui/inspector/polygon/polygon.yaml", error, capacity);
    if (!object->polygon_layout) {
        goto failed;
    }
    return true;
failed:
    vector_inspector_destroy(inspector);
    return false;
}

void vector_inspector_destroy(VectorInspector *inspector) {
    pui_layout_free(inspector->multi_layout);
    pui_layout_free(inspector->artboard.layout);
    pui_layout_free(inspector->object.layout);
    pui_layout_free(inspector->object.image_layout);
    pui_layout_free(inspector->object.gradient_layout);
    pui_layout_free(inspector->object.corners_layout);
    pui_layout_free(inspector->object.polygon_layout);
    pui_layout_free(inspector->object.text.layout);
    *inspector = (VectorInspector){0};
}
