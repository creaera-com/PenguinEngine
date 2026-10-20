/** @file @brief Canonical closed paths feed atomic object replacements, never native UI events. */
#include "operations.h"
#include "editors/vector/mask/mask.h"
#include "editors/vector/object/clipboard.h"
#include <math.h>
#include <stdlib.h>

bool vector_paths_can_operate(VectorDocument *d, VectorPathOperation op) {
    if (op < VECTOR_PATH_CONVERT || op > VECTOR_PATH_DIVIDE || !d->active_rectangle ||
        d->history.cursor >= VECTOR_HISTORY_CAPACITY) {
        return false;
    }
    int count = 0;
    unsigned parent = 0;
    bool convertible = false;
    for (int i = 0; i < d->rectangle_count; i++) {
        VectorShape *s = &d->rectangles[i];
        if (!vector_document_is_selected(d, s->id)) {
            continue;
        }
        if ((op >= VECTOR_PATH_UNION && (s->repeat.enabled || vector_mask_find(d, s->group_id))) ||
            *s->image.source || s->text.enabled ||
            vector_rectangle_effective_flags(d, s) & VECTOR_OBJECT_STATE_MASK) {
            return false;
        }
        if (op >= VECTOR_PATH_UNION && parent && parent != s->parent_id) {
            return false;
        }
        parent = s->parent_id;
        count++;
        if (op == VECTOR_PATH_STROKE && s->stroke.width <= 0) {
            return false;
        }
        if (s->curve.count && !s->curve.closed) {
            return false;
        }
        convertible |= !s->path.count || s->curve.count;
    }
    return op >= VECTOR_PATH_UNION ? count >= 2
                                   : count >= 1 && (op != VECTOR_PATH_CONVERT || convertible);
}

static bool store(VectorShape *shape, const VectorPath *path) {
    if (!path->count) {
        return false;
    }
    PUIRect bounds = vector_path_bounds(path);
    if (bounds.w <= .000001f || bounds.h <= .000001f) {
        return false;
    }
    // Keep the shared transform's minimum 1px box without changing the actual path geometry.
    bounds.w = fmaxf(1, bounds.w);
    bounds.h = fmaxf(1, bounds.h);
    shape->rect = bounds;
    shape->angle = 0;
    shape->curve_kind = VECTOR_CURVE_FREE;
    shape->curve = (VectorBezier){0};
    shape->path = *path;
    for (unsigned i = 0; i < path->count; i++) {
        shape->path.points[i].x = (path->points[i].x - bounds.x) / bounds.w;
        shape->path.points[i].y = (path->points[i].y - bounds.y) / bounds.h;
    }
    VectorShape candidate = *shape;
    candidate.id = candidate.id ? candidate.id : 1; // Fresh IDs are assigned by the transaction.
    return vector_rectangle_valid(candidate);
}

static bool add_components(VectorPath *pieces, int *count, const VectorPath *path) {
    int n = vector_path_components(path, pieces + *count, VECTOR_MAX_RECTANGLES - *count);
    if (n < 0) {
        return false;
    }
    *count += n;
    return true;
}

static int divide(VectorPath *inputs, int count, VectorPath *pieces) {
    VectorPath *next = calloc(VECTOR_MAX_RECTANGLES, sizeof *next);
    if (!next) {
        return -1;
    }
    int used = 0;
    bool ok = add_components(pieces, &used, &inputs[0]);
    for (int i = 1; i < count && ok; i++) {
        int added = 0;
        VectorPath remaining = inputs[i];
        for (int j = 0; j < used && ok; j++) {
            VectorPath inside, outside;
            ok = vector_path_combine(&pieces[j], &inputs[i], VECTOR_AREA_INTERSECTION, &inside) &&
                 vector_path_combine(&pieces[j], &inputs[i], VECTOR_AREA_DIFFERENCE, &outside) &&
                 vector_path_combine(&remaining, &pieces[j], VECTOR_AREA_DIFFERENCE, &remaining) &&
                 add_components(next, &added, &inside) && add_components(next, &added, &outside);
        }
        ok = ok && add_components(next, &added, &remaining);
        if (ok) {
            used = added;
            for (int j = 0; j < used; j++) {
                pieces[j] = next[j];
            }
        }
    }
    free(next);
    return ok ? used : -1;
}

bool vector_paths_operate(VectorDocument *d, VectorPathOperation op) {
    if (!vector_paths_can_operate(d, op)) {
        return false;
    }
    VectorShape *shapes = calloc(VECTOR_MAX_RECTANGLES, sizeof *shapes);
    VectorPath *inputs = calloc(VECTOR_MAX_RECTANGLES, sizeof *inputs);
    VectorPath *results = calloc(VECTOR_MAX_RECTANGLES, sizeof *results);
    if (!shapes || !inputs || !results) {
        free(shapes);
        free(inputs);
        free(results);
        return false;
    }
    int count = 0;
    for (int i = 0; i < d->rectangle_count; i++) {
        if (vector_document_is_selected(d, d->rectangles[i].id)) {
            shapes[count] = d->rectangles[i];
            inputs[count] =
                vector_shape_path(&shapes[count], shapes[count].rect, shapes[count].angle);
            count++;
        }
    }
    bool ok = true;
    int output = count;
    if (op == VECTOR_PATH_CONVERT) {
        for (int i = 0; i < count; i++) {
            if (shapes[i].curve.count) {
                VectorBezier curve = shapes[i].curve;
                shapes[i].curve_kind = VECTOR_CURVE_FREE;
                shapes[i].curve = (VectorBezier){0};
                bool converted = false;
                float tolerance = .001f;
                for (int attempt = 0; attempt < 12; attempt++, tolerance *= 2) {
                    if (!vector_bezier_flatten(&curve, tolerance, &shapes[i].path)) {
                        continue;
                    }
                    if (vector_path_contour_area(&shapes[i].path, 0) < 0) {
                        for (unsigned n = 0; n < shapes[i].path.count / 2; n++) {
                            VectorPoint swap = shapes[i].path.points[n];
                            shapes[i].path.points[n] =
                                shapes[i].path.points[shapes[i].path.count - 1 - n];
                            shapes[i].path.points[shapes[i].path.count - 1 - n] = swap;
                        }
                    }
                    if (vector_rectangle_valid(shapes[i])) {
                        converted = true;
                        break;
                    }
                }
                if (!converted) {
                    ok = false;
                    break;
                }
            }
            if (!shapes[i].path.count) {
                shapes[i].path = (VectorPath){.count = 4,
                                              .contours = 1,
                                              .points = {{0, 0}, {1, 0}, {1, 1}, {0, 1}},
                                              .ends = {4}};
            }
        }
    } else if (op == VECTOR_PATH_STROKE) {
        for (int i = 0; i < count && ok; i++) {
            VectorStroke stroke = shapes[i].stroke;
            int placement = stroke.alignment == VECTOR_STROKE_INSIDE    ? -1
                            : stroke.alignment == VECTOR_STROKE_OUTSIDE ? 1
                                                                        : 0;
            shapes[i].fill = stroke.color;
            shapes[i].stroke.width = 0;
            ok = vector_shape_outline(&shapes[i], shapes[i].rect, shapes[i].angle, stroke.width,
                                      placement, &results[i]) &&
                 store(&shapes[i], &results[i]);
        }
    } else {
        if (op == VECTOR_PATH_DIVIDE) {
            output = divide(inputs, count, results);
            ok = output >= 0;
        } else {
            VectorAreaOperation operation = op == VECTOR_PATH_UNION       ? VECTOR_AREA_UNION
                                            : op == VECTOR_PATH_SUBTRACT  ? VECTOR_AREA_DIFFERENCE
                                            : op == VECTOR_PATH_INTERSECT ? VECTOR_AREA_INTERSECTION
                                                                          : VECTOR_AREA_XOR;
            results[0] = inputs[0];
            for (int i = 1; i < count && ok; i++) {
                ok = vector_path_combine(&results[0], &inputs[i], operation, &results[0]);
            }
            output = results[0].count ? 1 : 0;
        }
        VectorShape style = shapes[0];
        for (int i = 0; i < output && ok; i++) {
            shapes[i] = style;
            shapes[i].id = 0;
            ok = store(&shapes[i], &results[i]);
        }
    }
    if (ok) {
        ok = vector_objects_replace_selection(d, shapes, output);
    }
    free(results);
    free(inputs);
    free(shapes);
    return ok;
}
