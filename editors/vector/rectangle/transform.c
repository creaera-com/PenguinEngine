/** @file @brief One group transform is one history command with atomic apply/revert. */
#include "transform.h"
#include "editors/vector/document/document.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    unsigned id;
    PUIRect before, after;
    float before_angle, after_angle;
} TransformItem;

typedef struct {
    int count;
    TransformItem items[VECTOR_MAX_RECTANGLES];
} Transform;

static bool valid_rect(PUIRect r) {
    return isfinite(r.x) && isfinite(r.y) && isfinite(r.w) && isfinite(r.h) &&
           fabsf(r.x) <= VECTOR_MAX_ARTBOARD_SIZE && fabsf(r.y) <= VECTOR_MAX_ARTBOARD_SIZE &&
           r.w >= 1 && r.h >= 1 && r.w <= VECTOR_MAX_ARTBOARD_SIZE &&
           r.h <= VECTOR_MAX_ARTBOARD_SIZE;
}

static bool replace(VectorDocument *document, const Transform *command, bool revert) {
    for (int i = 0; i < command->count; i++) {
        const TransformItem *item = &command->items[i];
        VectorRectangle *rectangle = vector_rectangle_find(document, item->id);
        const PUIRect *before = revert ? &item->after : &item->before;
        if (!rectangle || memcmp(&rectangle->rect, before, sizeof *before) ||
            rectangle->angle != (revert ? item->after_angle : item->before_angle)) {
            return false;
        }
    }
    for (int i = 0; i < command->count; i++) {
        const TransformItem *item = &command->items[i];
        VectorRectangle *rectangle = vector_rectangle_find(document, item->id);
        rectangle->rect = revert ? item->before : item->after;
        rectangle->angle = revert ? item->before_angle : item->after_angle;
    }
    return true;
}

static bool apply(void *document, const void *command) {
    return replace(document, command, false);
}

static bool revert(void *document, const void *command) {
    return replace(document, command, true);
}

bool vector_rectangles_set_transforms(VectorDocument *document, const unsigned *ids,
                                      const PUIRect *rects, const float *angles, int count) {
    if (!ids || !rects || count < 1 || count > VECTOR_MAX_RECTANGLES) {
        return false;
    }
    Transform *command = calloc(1, sizeof *command);
    if (!command) {
        return false;
    }
    command->count = count;
    bool changed = false;
    for (int i = 0; i < count; i++) {
        VectorRectangle *rectangle = vector_rectangle_find(document, ids[i]);
        bool valid =
            rectangle &&
            !(vector_rectangle_effective_flags(document, rectangle) & VECTOR_OBJECT_STATE_MASK) &&
            valid_rect(rects[i]) &&
            (!angles || (isfinite(angles[i]) && angles[i] >= -180 && angles[i] < 180));
        for (int j = 0; j < i; j++) {
            valid = valid && ids[j] != ids[i];
        }
        if (!valid) {
            free(command);
            return false;
        }
        command->items[i] = (TransformItem){ids[i], rectangle->rect, rects[i], rectangle->angle,
                                            angles ? angles[i] : rectangle->angle};
        changed |= memcmp(&rectangle->rect, &rects[i], sizeof rects[i]) != 0 ||
                   rectangle->angle != command->items[i].after_angle;
    }
    if (!changed) {
        free(command);
        return true;
    }
    if (!vector_history_execute(&document->history, document,
                                (VectorCommand){apply, revert, command})) {
        free(command);
        return false;
    }
    return true;
}

bool vector_rectangles_set_rects(VectorDocument *document, const unsigned *ids,
                                 const PUIRect *rects, int count) {
    return vector_rectangles_set_transforms(document, ids, rects, NULL, count);
}

bool vector_rectangles_translate(VectorDocument *document, const unsigned *ids, int count, float dx,
                                 float dy) {
    if (!ids || count < 1 || count > VECTOR_MAX_RECTANGLES || !isfinite(dx) || !isfinite(dy)) {
        return false;
    }
    PUIRect rects[VECTOR_MAX_RECTANGLES];
    for (int i = 0; i < count; i++) {
        VectorRectangle *rectangle = vector_rectangle_find(document, ids[i]);
        if (!rectangle) {
            return false;
        }
        rects[i] = rectangle->rect;
        rects[i].x += dx;
        rects[i].y += dy;
    }
    return vector_rectangles_set_rects(document, ids, rects, count);
}

bool vector_rectangles_transform_write(const VectorCommand *command, FILE *file) {
    if (command->apply != apply || command->revert != revert || !command->data) {
        return false;
    }
    unsigned kind = 5;
    const Transform *translation = command->data;
    return fwrite(&kind, sizeof kind, 1, file) == 1 &&
           fwrite(&translation->count, sizeof translation->count, 1, file) == 1 &&
           fwrite(translation->items, sizeof *translation->items, translation->count, file) ==
               (size_t)translation->count;
}

bool vector_rectangles_transform_read(VectorCommand *command, FILE *file) {
    Transform *owned = calloc(1, sizeof *owned);
    if (!owned) {
        return false;
    }
    bool valid = fread(&owned->count, sizeof owned->count, 1, file) == 1 && owned->count > 0 &&
                 owned->count <= VECTOR_MAX_RECTANGLES;
    if (valid) {
        valid =
            fread(owned->items, sizeof *owned->items, owned->count, file) == (size_t)owned->count;
    }
    for (int i = 0; valid && i < owned->count; i++) {
        TransformItem *item = &owned->items[i];
        valid = item->id && valid_rect(item->before) && valid_rect(item->after) &&
                isfinite(item->before_angle) && item->before_angle >= -180 &&
                item->before_angle < 180 && isfinite(item->after_angle) &&
                item->after_angle >= -180 && item->after_angle < 180;
        for (int j = 0; j < i; j++) {
            valid = valid && owned->items[j].id != item->id;
        }
    }
    if (!valid) {
        free(owned);
        return false;
    }
    *command = (VectorCommand){apply, revert, owned};
    return true;
}
