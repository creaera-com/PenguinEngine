/** @file @brief Geometry-only smart guides; no editor, renderer, model or history dependency. */
#include "element_snap.h"
#include <math.h>

typedef struct {
    bool found, spacing;
    float delta, anchor;
    int first, second;
} Match;

static bool valid(PUIRect r) {
    return isfinite(r.x) && isfinite(r.y) && isfinite(r.w) && isfinite(r.h) && r.w > 0 && r.h > 0;
}

static float start(PUIRect r, bool vertical) {
    return vertical ? r.y : r.x;
}

static float size(PUIRect r, bool vertical) {
    return vertical ? r.h : r.w;
}

static void consider(Match *match, float delta, float anchor, int first, int second, bool spacing,
                     float tolerance) {
    if (fabsf(delta) <= tolerance &&
        (!match->found || fabsf(delta) < fabsf(match->delta) - .0001f)) {
        *match = (Match){true, spacing, delta, anchor, first, second};
    }
}

static Match match_axis(PUIRect moving, const PUIRect *targets, int count, float tolerance,
                        bool vertical) {
    Match match = {0};
    float origin = start(moving, vertical), length = size(moving, vertical);
    for (int i = 0; i < count; i++) {
        if (!valid(targets[i])) {
            continue;
        }
        for (int a = 0; a < 3; a++) {
            float anchor = start(targets[i], vertical) + size(targets[i], vertical) * a / 2;
            for (int b = 0; b < 3; b++) {
                consider(&match, anchor - origin - length * b / 2, anchor, i, -1, false, tolerance);
            }
        }
    }
    /* Equal gaps use the immediate neighbours, never an arbitrary outer pair. */
    int left = -1, right = -1;
    float left_end = -INFINITY, right_start = INFINITY;
    for (int i = 0; i < count; i++) {
        PUIRect target = targets[i];
        bool overlap =
            start(target, !vertical) < start(moving, !vertical) + size(moving, !vertical) &&
            start(target, !vertical) + size(target, !vertical) > start(moving, !vertical);
        if (!valid(target) || !overlap) {
            continue;
        }
        float a = start(target, vertical), b = a + size(target, vertical);
        if (b <= origin + tolerance && b > left_end) {
            left = i;
            left_end = b;
        }
        if (a >= origin + length - tolerance && a < right_start) {
            right = i;
            right_start = a;
        }
    }
    if (left >= 0 && right >= 0 && left != right && right_start - left_end >= length) {
        float desired = left_end + (right_start - left_end - length) / 2;
        consider(&match, desired - origin, 0, left, right, true, tolerance);
    }
    return match;
}

static void add_guide(VectorElementSnap *result, bool vertical, float a, float b, float coordinate,
                      bool spacing) {
    VectorSnapGuide line = vertical ? (VectorSnapGuide){coordinate, a, coordinate, b, spacing}
                                    : (VectorSnapGuide){a, coordinate, b, coordinate, spacing};
    result->guides[result->guide_count++] = line;
}

static void guides(VectorElementSnap *result, PUIRect moving, const PUIRect *targets, Match match,
                   bool vertical) {
    if (!match.found) {
        return;
    }
    PUIRect first = targets[match.first];
    if (match.spacing) {
        float coordinate = start(moving, !vertical) + size(moving, !vertical) / 2;
        add_guide(result, vertical, start(first, vertical) + size(first, vertical),
                  start(moving, vertical), coordinate, true);
        add_guide(result, vertical, start(moving, vertical) + size(moving, vertical),
                  start(targets[match.second], vertical), coordinate, true);
    } else {
        float a = fminf(start(moving, !vertical), start(first, !vertical));
        float b = fmaxf(start(moving, !vertical) + size(moving, !vertical),
                        start(first, !vertical) + size(first, !vertical));
        add_guide(result, !vertical, a, b, match.anchor, false);
    }
}

VectorElementSnap vector_element_snap(PUIRect moving, const PUIRect *targets, int count,
                                      float tolerance) {
    VectorElementSnap result = {0};
    if (!valid(moving) || !targets || count <= 0 || !isfinite(tolerance) || tolerance < 0) {
        return result;
    }
    Match x = match_axis(moving, targets, count, tolerance, false);
    Match y = match_axis(moving, targets, count, tolerance, true);
    result.snapped_x = x.found;
    result.snapped_y = y.found;
    result.dx = x.found ? x.delta : 0;
    result.dy = y.found ? y.delta : 0;
    moving.x += result.dx;
    moving.y += result.dy;
    guides(&result, moving, targets, x, false);
    guides(&result, moving, targets, y, true);
    return result;
}
