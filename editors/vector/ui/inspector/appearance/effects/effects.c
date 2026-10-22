/** @file @brief Shadow inspector bindings; all editable values remain in the shape model. */
#include "effects.h"
#include "editors/vector/object/commands.h"
#include "editors/vector/workspace/selection.h"
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

PUILayout *vector_effects_layout(const EditorServices *services, PUILayout *base, char *error,
                                 size_t capacity) {
    PUILayout *effects = editor_load_ui(
        services, "editors/vector/ui/inspector/appearance/effects/effects.yaml", error, capacity);
    PUILayout *result = base && effects
                            ? pui_layout_compose(base, "object_effects", effects, error, capacity)
                            : NULL;
    pui_layout_free(base);
    pui_layout_free(effects);
    return result;
}

static bool number(PUIView *view, const char *id, float *out) {
    const char *text = pui_view_text(view, id);
    char *end = NULL;
    errno = 0;
    if (!text || !*text) {
        return false;
    }
    *out = strtof(text, &end);
    return !errno && end != text && !*end && isfinite(*out);
}

bool vector_effects_action(VectorDocument *d, unsigned id, PUIView *view, const char *action) {
    const char *prefix = "vector.shadow.";
    if (strncmp(action, prefix, strlen(prefix))) {
        return false;
    }
    VectorShape *source = d ? vector_rectangle_find(d, id) : NULL;
    if (!source) {
        return true;
    }
    VectorShape value = *source;
    VectorShadow *s = &value.shadow;
    const char *field = action + strlen(prefix);
    bool valid = true;
    if (!strcmp(field, "enabled")) {
        bool enabled = pui_view_checked(view, "shadow_enabled");
        if (enabled && !s->color && !s->blur && !s->offset_x && !s->offset_y && !s->spread) {
            *s = vector_shadow_default();
        }
        s->enabled = enabled;
    } else if (!strcmp(field, "color")) {
        valid = pui_view_color(view, "shadow_color", &s->color);
    } else if (!strcmp(field, "x")) {
        valid = number(view, "shadow_x", &s->offset_x);
    } else if (!strcmp(field, "y")) {
        valid = number(view, "shadow_y", &s->offset_y);
    } else if (!strcmp(field, "blur")) {
        valid = number(view, "shadow_blur", &s->blur);
    } else if (!strcmp(field, "spread")) {
        valid = number(view, "shadow_spread", &s->spread);
    } else {
        return false;
    }
    vector_selection_cancel(d);
    bool ok = valid && vector_shape_update(d, &value);
    pui_view_set_text(view, "status",
                      ok ? "Schlagschatten aktualisiert"
                         : "Schatten unveraendert: Objekt gesperrt oder Wert ungueltig");
    return true;
}

void vector_effects_sync(PUIView *view, const VectorShadow *s, const VectorShadow *old,
                         bool reset) {
    pui_view_set_checked(view, "shadow_enabled", s->enabled != 0);
    if (reset || s->color != old->color) {
        pui_view_set_color(view, "shadow_color", s->color);
    }
    const char *ids[] = {"shadow_x", "shadow_y", "shadow_blur", "shadow_spread"};
    float values[] = {s->offset_x, s->offset_y, s->blur, s->spread};
    float prior[] = {old->offset_x, old->offset_y, old->blur, old->spread};
    for (unsigned i = 0; i < 4; i++) {
        if (reset || values[i] != prior[i]) {
            char text[32];
            snprintf(text, sizeof text, "%.7g", (double)values[i]);
            pui_view_set_text(view, ids[i], text);
        }
    }
}
