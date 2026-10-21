/** @file @brief Rectangle creation and before/after edits, independent of editor UI. */
#include "rectangle.h"
#include "editors/vector/document/document.h"
#include "editors/vector/geometry/rotation.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    VectorRectangle rectangle;
    int index;
    unsigned previous_board, previous_rectangle;
} AddRectangle;

typedef struct {
    VectorRectangle before, after;
} EditRectangle;

VectorRectangle *vector_rectangle_find(VectorDocument *document, unsigned id) {
    for (int i = 0; i < document->rectangle_count; i++) {
        if (document->rectangles[i].id == id) {
            return &document->rectangles[i];
        }
    }
    return NULL;
}

unsigned vector_rectangle_effective_flags(VectorDocument *document,
                                          const VectorRectangle *rectangle) {
    VectorArtboard *parent = vector_artboard_find(document, rectangle->parent_id);
    return rectangle->flags | (parent ? parent->flags : VECTOR_OBJECT_STATE_MASK);
}

bool vector_rectangle_valid(VectorRectangle rectangle) {
    PUIRect r = rectangle.rect;
    if (!vector_gradient_valid(&rectangle.gradient) || !vector_shadow_valid(&rectangle.shadow) ||
        !vector_repeat_valid(&rectangle.repeat) ||
        (rectangle.repeat.enabled && rectangle.clip_mask) ||
        !vector_image_valid(&rectangle.image) ||
        (*rectangle.image.source &&
         (rectangle.text.enabled || rectangle.clip_mask || rectangle.curve.count ||
          rectangle.path.count || rectangle.curve_kind)) ||
        rectangle.clip_mask > 1 ||
        (rectangle.clip_mask && (!rectangle.group_id || rectangle.text.enabled ||
                                 (rectangle.curve.count && !rectangle.curve.closed))) ||
        !vector_text_valid(&rectangle.text) ||
        (rectangle.text.enabled &&
         (rectangle.curve.count || rectangle.path.count || rectangle.curve_kind)) ||
        !vector_path_valid(&rectangle.path) || !vector_bezier_valid(&rectangle.curve) ||
        rectangle.curve.count == 1 || rectangle.curve_kind > VECTOR_CURVE_POLYGON ||
        (rectangle.curve_kind == VECTOR_CURVE_POLYGON &&
         (rectangle.curve.count < 3 || !rectangle.curve.closed)) ||
        (rectangle.curve_kind == VECTOR_CURVE_ELLIPSE &&
         (rectangle.curve.count != 4 || !rectangle.curve.closed))) {
        return false;
    }
    for (unsigned i = 0; i < rectangle.path.count; i++) {
        VectorPoint p = rectangle.path.points[i];
        if (p.x < -.00001f || p.x > 1.00001f || p.y < -.00001f || p.y > 1.00001f) {
            return false;
        }
    }
    bool valid =
        isfinite(rectangle.corner_radius) && rectangle.corner_radius >= 0 &&
        rectangle.corner_radius <= VECTOR_MAX_ARTBOARD_SIZE && isfinite(rectangle.stroke.width) &&
        rectangle.stroke.width >= 0 && rectangle.stroke.width <= VECTOR_MAX_ARTBOARD_SIZE &&
        rectangle.stroke.alignment >= VECTOR_STROKE_CENTER &&
        rectangle.stroke.alignment <= VECTOR_STROKE_OUTSIDE && isfinite(rectangle.angle) &&
        rectangle.angle >= -360 && rectangle.angle <= 360 && rectangle.id && rectangle.parent_id &&
        !(rectangle.flags & ~VECTOR_OBJECT_STATE_MASK) &&
        memchr(rectangle.name, 0, sizeof rectangle.name) && *rectangle.name && isfinite(r.x) &&
        isfinite(r.y) && fabsf(r.x) <= VECTOR_MAX_ARTBOARD_SIZE &&
        fabsf(r.y) <= VECTOR_MAX_ARTBOARD_SIZE && isfinite(r.w) && isfinite(r.h) && r.w >= 1 &&
        r.h >= 1 && r.w <= VECTOR_MAX_ARTBOARD_SIZE && r.h <= VECTOR_MAX_ARTBOARD_SIZE;
    if (valid && !rectangle.curve.count && rectangle.path.count && rectangle.stroke.width > 0) {
        VectorPath path = vector_shape_path(&rectangle, rectangle.rect, rectangle.angle), outline;
        int placement = rectangle.stroke.alignment == VECTOR_STROKE_INSIDE    ? -1
                        : rectangle.stroke.alignment == VECTOR_STROKE_OUTSIDE ? 1
                                                                              : 0;
        valid = vector_path_outline(&path, rectangle.stroke.width, placement, &outline);
    }
    return valid;
}

static bool add_apply(void *state, const void *payload) {
    VectorDocument *document = state;
    const AddRectangle *command = payload;
    if (document->rectangle_count >= VECTOR_MAX_RECTANGLES ||
        document->rectangle_count != command->index ||
        !vector_artboard_find(document, command->rectangle.parent_id)) {
        return false;
    }
    document->rectangles[document->rectangle_count++] = command->rectangle;
    document->active_artboard = command->rectangle.parent_id;
    document->active_rectangle = command->rectangle.id;
    return true;
}

static bool add_revert(void *state, const void *payload) {
    VectorDocument *document = state;
    const AddRectangle *command = payload;
    if (document->rectangle_count != command->index + 1 ||
        document->rectangles[command->index].id != command->rectangle.id) {
        return false;
    }
    document->rectangle_count--;
    document->active_artboard = command->previous_board;
    document->active_rectangle = command->previous_rectangle;
    return true;
}

static bool replace(void *state, const VectorRectangle *before, const VectorRectangle *after) {
    VectorRectangle *rectangle = vector_rectangle_find(state, before->id);
    if (!rectangle || memcmp(rectangle, before, sizeof *before)) {
        return false;
    }
    *rectangle = *after;
    return true;
}

static bool edit_apply(void *state, const void *payload) {
    const EditRectangle *edit = payload;
    return replace(state, &edit->before, &edit->after);
}

static bool edit_revert(void *state, const void *payload) {
    const EditRectangle *edit = payload;
    return replace(state, &edit->after, &edit->before);
}

bool vector_rectangle_add(VectorDocument *document, unsigned parent_id, PUIRect rect) {
    VectorArtboard *parent = vector_artboard_find(document, parent_id);
    VectorRectangle value = {.id = document->next_rectangle_id,
                             .parent_id = parent_id,
                             .fill = VECTOR_RECTANGLE_DEFAULT_FILL,
                             .stroke = {.color = 0x303030ffu},
                             .rect = rect};
    snprintf(value.name, sizeof value.name, "Rechteck %u", value.id);
    if (!parent || parent->flags & VECTOR_OBJECT_STATE_MASK || !vector_rectangle_valid(value) ||
        rect.x < 0 || rect.y < 0 || rect.x + rect.w > parent->rect.w ||
        rect.y + rect.h > parent->rect.h || document->rectangle_count >= VECTOR_MAX_RECTANGLES) {
        return false;
    }
    AddRectangle *command = calloc(1, sizeof *command);
    if (!command) {
        return false;
    }
    *command = (AddRectangle){value, document->rectangle_count, document->active_artboard,
                              document->active_rectangle};
    if (!vector_history_execute(&document->history, document,
                                (VectorCommand){add_apply, add_revert, command})) {
        free(command);
        return false;
    }
    document->next_rectangle_id++;
    return true;
}

static bool edit(VectorDocument *document, VectorRectangle before, VectorRectangle after) {
    if (!vector_rectangle_valid(after)) {
        return false;
    }
    if (!memcmp(&before, &after, sizeof before)) {
        return true;
    }
    EditRectangle *command = calloc(1, sizeof *command);
    if (!command) {
        return false;
    }
    *command = (EditRectangle){before, after};
    if (vector_history_execute(&document->history, document,
                               (VectorCommand){edit_apply, edit_revert, command})) {
        return true;
    }
    free(command);
    return false;
}

bool vector_rectangle_update(VectorDocument *document, unsigned id, const char *name,
                             PUIRect rect) {
    VectorRectangle *rectangle = vector_rectangle_find(document, id);
    if (!rectangle || !name || !*name || strlen(name) >= sizeof rectangle->name ||
        (vector_rectangle_effective_flags(document, rectangle) & VECTOR_OBJECT_LOCKED)) {
        return false;
    }
    VectorRectangle after = *rectangle;
    after.rect = rect;
    snprintf(after.name, sizeof after.name, "%s", name);
    return edit(document, *rectangle, after);
}

bool vector_rectangle_set_angle(VectorDocument *document, unsigned id, float degrees) {
    VectorRectangle *rectangle = vector_rectangle_find(document, id);
    if (!rectangle || !isfinite(degrees) ||
        (vector_rectangle_effective_flags(document, rectangle) & VECTOR_OBJECT_LOCKED)) {
        return false;
    }
    VectorRectangle after = *rectangle;
    after.angle = vector_angle_turn(degrees);
    return edit(document, *rectangle, after);
}

bool vector_rectangle_set_radius(VectorDocument *document, unsigned id, float radius) {
    VectorRectangle *rectangle = vector_rectangle_find(document, id);
    if (!rectangle || rectangle->curve.count || rectangle->path.count || rectangle->text.enabled ||
        *rectangle->image.source ||
        (vector_rectangle_effective_flags(document, rectangle) & VECTOR_OBJECT_LOCKED)) {
        return false;
    }
    VectorRectangle after = *rectangle;
    after.corner_radius = radius;
    return edit(document, *rectangle, after);
}

bool vector_rectangle_set_fill(VectorDocument *document, unsigned id, uint32_t rgba) {
    VectorRectangle *rectangle = vector_rectangle_find(document, id);
    if (!rectangle ||
        (vector_rectangle_effective_flags(document, rectangle) & VECTOR_OBJECT_LOCKED)) {
        return false;
    }
    VectorRectangle after = *rectangle;
    after.fill = rgba;
    return edit(document, *rectangle, after);
}

bool vector_rectangle_set_stroke(VectorDocument *document, unsigned id, VectorStroke stroke) {
    VectorRectangle *rectangle = vector_rectangle_find(document, id);
    if (!rectangle ||
        (vector_rectangle_effective_flags(document, rectangle) & VECTOR_OBJECT_LOCKED)) {
        return false;
    }
    VectorRectangle after = *rectangle;
    after.stroke = stroke;
    return edit(document, *rectangle, after);
}

bool vector_rectangle_set_state(VectorDocument *document, unsigned id, unsigned flags) {
    VectorRectangle *rectangle = vector_rectangle_find(document, id);
    if (!rectangle) {
        return false;
    }
    VectorRectangle after = *rectangle;
    after.flags = flags;
    return edit(document, *rectangle, after);
}

bool vector_rectangle_command_write(const VectorCommand *command, FILE *file) {
    unsigned kind;
    if (command->apply == add_apply && command->revert == add_revert && command->data) {
        kind = 3;
        return fwrite(&kind, sizeof kind, 1, file) == 1 &&
               fwrite(command->data, sizeof(AddRectangle), 1, file) == 1;
    }
    if (command->apply == edit_apply && command->revert == edit_revert && command->data) {
        kind = 4;
        return fwrite(&kind, sizeof kind, 1, file) == 1 &&
               fwrite(command->data, sizeof(EditRectangle), 1, file) == 1;
    }
    return false;
}

bool vector_rectangle_command_read(VectorCommand *command, FILE *file, unsigned kind) {
    if (kind == 3) {
        AddRectangle value;
        if (fread(&value, sizeof value, 1, file) != 1 || !vector_rectangle_valid(value.rectangle) ||
            value.index < 0 || value.index >= VECTOR_MAX_RECTANGLES) {
            return false;
        }
        AddRectangle *owned = malloc(sizeof *owned);
        if (!owned) {
            return false;
        }
        *owned = value;
        *command = (VectorCommand){add_apply, add_revert, owned};
        return true;
    }
    if (kind == 4) {
        EditRectangle value;
        if (fread(&value, sizeof value, 1, file) != 1 || !vector_rectangle_valid(value.before) ||
            !vector_rectangle_valid(value.after) || value.before.id != value.after.id ||
            value.before.parent_id != value.after.parent_id) {
            return false;
        }
        EditRectangle *owned = malloc(sizeof *owned);
        if (!owned) {
            return false;
        }
        *owned = value;
        *command = (VectorCommand){edit_apply, edit_revert, owned};
        return true;
    }
    return false;
}
