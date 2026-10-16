#include "penguin/ui.h"
#include <math.h>
#include <string.h>

static float clamp(float v, float low, float high) {
    return fmaxf(low, fminf(high, v));
}

UiId ui_id(const char *s) {
    uint32_t hash = 2166136261u;
    while (*s) {
        hash ^= (unsigned char)*s++;
        hash *= 16777619u;
    }

    return hash ? hash : 1;
}

void ui_init(UiContext *c) {
    memset(c, 0, sizeof *c);
}

bool ui_contains(UiRect r, float x, float y) {
    return r.w > 0 && r.h > 0 && x >= r.x && y >= r.y && x < r.x + r.w && y < r.y + r.h;
}

static bool visible(const UiContext *c, float x, float y) {
    return !c->clip_depth || ui_contains(c->clips[c->clip_depth - 1], x, y);
}

bool ui_hovered(const UiContext *c, UiRect r) {
    return !c->blocked && visible(c, c->x, c->y) && ui_contains(r, c->x, c->y);
}

void ui_pointer(UiContext *c, float x, float y, bool down) {
    if (!isfinite(x) || !isfinite(y)) {
        return;
    }
    c->x = x;
    c->y = y;
    if (down && !c->down) {
        c->pressed = true;
        c->press_x = x;
        c->press_y = y;
    }
    if (!down && c->down) {
        c->released = true;
        c->release_x = x;
        c->release_y = y;
    }
    c->down = down;
}

void ui_cancel(UiContext *c) {
    c->active = 0;
    c->down = c->pressed = c->released = c->activate = c->dragging = false;
}

void ui_focus(UiContext *c, UiId id) {
    c->focus = id;
}

void ui_activate(UiContext *c) {
    c->activate = true;
}

void ui_end_frame(UiContext *c) {
    if (c->active && !c->active_seen) {
        c->active = 0;
    }
    c->pressed = c->released = c->activate = c->active_seen = false;
    c->clip_depth = 0;
}

bool ui_clip_push(UiContext *c, UiRect r) {
    if (c->clip_depth == UI_MAX_CLIPS) {
        return false;
    }
    if (c->clip_depth) {
        UiRect p = c->clips[c->clip_depth - 1];
        float x = fmaxf(r.x, p.x), y = fmaxf(r.y, p.y);
        r = (UiRect){x, y, fmaxf(0, fminf(r.x + r.w, p.x + p.w) - x),
                     fmaxf(0, fminf(r.y + r.h, p.y + p.h) - y)};
    }
    c->clips[c->clip_depth++] = r;
    return true;
}

void ui_clip_pop(UiContext *c) {
    if (c->clip_depth) {
        c->clip_depth--;
    }
}

static bool acquire(UiContext *c, UiId id, UiRect r) {
    if (c->blocked || !id) {
        return false;
    }
    if (c->active == id) {
        c->active_seen = true;
    }
    if (c->pressed && !c->active && visible(c, c->press_x, c->press_y) &&
        ui_contains(r, c->press_x, c->press_y)) {
        c->active = id;
        c->pressed = false;
        c->active_seen = true;
        c->dragging = false;
        return true;
    }

    return false;
}

bool ui_button(UiContext *c, UiId id, UiRect r, bool enabled) {
    if (!enabled || c->blocked) {
        if (c->active == id) {
            c->active = 0;
        }
        return false;
    }
    if (acquire(c, id, r)) {
        c->focus = id;
    }
    if (c->released && c->active == id) {
        bool hit =
            visible(c, c->release_x, c->release_y) && ui_contains(r, c->release_x, c->release_y);
        c->active = 0;
        return hit;
    }

    bool in_clip = true;
    if (c->clip_depth) {
        UiRect p = c->clips[c->clip_depth - 1];
        in_clip = r.x + r.w > p.x && r.y + r.h > p.y && r.x < p.x + p.w && r.y < p.y + p.h &&
                  p.w > 0 && p.h > 0;
    }
    if (c->activate && c->focus == id && in_clip) {
        c->activate = false;
        return true;
    }

    return false;
}

bool ui_disclosure(UiContext *c, UiId id, UiRect r, bool *open, bool enabled) {
    if (!ui_button(c, id, r, enabled)) {
        return false;
    }
    *open = !*open;
    return true;
}

bool ui_drag_value(UiContext *c, UiId id, UiRect r, float *value, float low, float high,
                   UiAxis axis, float scale) {
    if (high < low) {
        high = low;
    }
    *value = clamp(*value, low, high);
    if (acquire(c, id, r)) {
        c->drag_origin = axis == UI_AXIS_X ? c->press_x : c->press_y;
        c->drag_value = *value;
    }
    if (c->blocked || c->active != id) {
        return false;
    }

    float old = *value, position = axis == UI_AXIS_X ? c->x : c->y;
    *value = clamp(c->drag_value + (position - c->drag_origin) * scale, low, high);
    if (c->released) {
        c->active = 0;
    }

    return old != *value;
}

void ui_columns_fit(float total, float *left, float *right, float ml, float mr, float mc) {
    if (!isfinite(total) || total <= 0) {
        *left = *right = 0;
        return;
    }
    ml = fmaxf(0, ml);
    mr = fmaxf(0, mr);
    mc = fmaxf(0, mc);
    float available = fmaxf(0, total - fminf(total, mc));
    if (available < ml + mr) {
        float ratio = ml + mr > 0 ? available / (ml + mr) : 0;
        *left = ml * ratio;
        *right = mr * ratio;
        return;
    }
    *left = clamp(*left, ml, available - mr);
    *right = clamp(*right, mr, available - *left);
}

float ui_scroll_max(const UiScroll *s) {
    return fmaxf(0, s->content - fmaxf(0, s->viewport));
}

void ui_scroll_clamp(UiScroll *s) {
    s->offset = clamp(s->offset, 0, ui_scroll_max(s));
}

void ui_scroll_by(UiScroll *s, float delta) {
    if (isfinite(delta)) {
        s->offset += delta;
    }
    ui_scroll_clamp(s);
}

UiRect ui_scroll_thumb(const UiScroll *s, UiRect r, UiAxis axis) {
    float length = axis == UI_AXIS_X ? r.w : r.h;
    float size = s->content > 0 ? length * fminf(1, s->viewport / s->content) : length;
    size = clamp(size, fminf(20, length), length);
    float max = ui_scroll_max(s),
          position = max > 0 ? clamp(s->offset, 0, max) / max * (length - size) : 0;
    if (axis == UI_AXIS_X) {
        r.x += position;
        r.w = size;
    } else {
        r.y += position;
        r.h = size;
    }

    return r;
}

bool ui_scrollbar(UiContext *c, UiId id, UiRect track, UiScroll *s, UiAxis axis) {
    ui_scroll_clamp(s);
    if (ui_scroll_max(s) <= 0) {
        if (c->active == id) {
            c->active = 0;
        }
        return false;
    }

    UiRect thumb = ui_scroll_thumb(s, track, axis);
    float old = s->offset;
    if (acquire(c, id, track)) {
        c->dragging = ui_contains(thumb, c->press_x, c->press_y);
        c->drag_origin = axis == UI_AXIS_X ? c->press_x : c->press_y;
        c->drag_value = s->offset;
        if (!c->dragging) {
            float p = axis == UI_AXIS_X ? c->press_x : c->press_y,
                  t = axis == UI_AXIS_X ? thumb.x : thumb.y;
            ui_scroll_by(s, p < t ? -s->viewport : s->viewport);
        }
    }
    if (c->active == id && !c->blocked) {
        if (c->dragging) {
            float length = axis == UI_AXIS_X ? track.w : track.h,
                  thumb_length = axis == UI_AXIS_X ? thumb.w : thumb.h,
                  p = axis == UI_AXIS_X ? c->x : c->y;
            if (length > thumb_length) {
                s->offset = clamp(c->drag_value + (p - c->drag_origin) * ui_scroll_max(s) /
                                                      (length - thumb_length),
                                  0, ui_scroll_max(s));
            }
        }
        if (c->released) {
            c->active = 0;
        }
    }

    return old != s->offset;
}

bool ui_tabs_init(UiTabs *t, const UiTab *items, int count, UiId selected) {
    if (count < 1 || count > UI_MAX_TABS) {
        return false;
    }

    bool found = false;
    for (int i = 0; i < count; i++) {
        if (!items[i].id || !isfinite(items[i].width) || items[i].width < 0 ||
            (items[i].width > 0 && items[i].width <= UI_TAB_GAP)) {
            return false;
        }
        for (int k = 0; k < i; k++) {
            if (items[i].id == items[k].id) {
                return false;
            }
        }
        if (items[i].id == selected) {
            found = true;
        }
    }
    if (!found) {
        return false;
    }
    memset(t, 0, sizeof *t);
    memcpy(t->items, items, (size_t)count * sizeof *items);
    t->count = count;
    t->gap = UI_TAB_GAP;
    t->selected = selected;
    return true;
}

bool ui_tabs_set_width(UiTabs *t, UiId id, float width) {
    if (!isfinite(width) || width < 0 || (width > 0 && width <= UI_TAB_GAP)) {
        return false;
    }
    for (int i = 0; i < t->count; i++) {
        if (t->items[i].id == id) {
            t->items[i].width = width;
            return true;
        }
    }

    return false;
}

static float tab_width(const UiTabs *t, float fallback, int index) {
    return t->items[index].width > 0 ? t->items[index].width : fallback;
}

UiRect ui_tab_rect(const UiTabs *t, UiRect bar, float width, int index) {
    if (index < 0 || index >= t->count) {
        return (UiRect){0};
    }

    float x = bar.x - t->scroll;
    for (int i = 0; i < index; i++) {
        x += tab_width(t, width, i);
    }

    return (UiRect){x, bar.y, tab_width(t, width, index) - t->gap, bar.h};
}

void ui_tabs_scroll(UiTabs *t, float viewport, float width, float delta) {
    float content = 0;
    for (int i = 0; i < t->count; i++) {
        content += tab_width(t, width, i);
    }
    if (t->count) {
        content -= t->gap;
    }

    UiScroll s = {t->scroll, content, viewport};
    ui_scroll_by(&s, delta);
    t->scroll = s.offset;
}

bool ui_tabs_update(UiContext *c, UiTabs *t, UiRect bar, float width) {
    if (c->blocked || width <= 0) {
        return false;
    }

    UiId before = t->selected;
    ui_tabs_scroll(t, bar.w, width, 0);
    for (int i = 0; i < t->count; i++) {
        UiTab tab = t->items[i];
        UiRect r = ui_tab_rect(t, bar, width, i);
        if (!tab.enabled) {
            if (c->active == tab.id) {
                c->active = 0;
            }
            continue;
        }
        if (!ui_contains(bar, c->press_x, c->press_y) && c->active != tab.id) {
            continue;
        }
        if (acquire(c, tab.id, r)) {
            c->focus = tab.id;
            c->drag_origin = c->press_x;
            t->dragging = false;
        }
        if (c->active != tab.id) {
            continue;
        }
        if (fabsf(c->x - c->drag_origin) > 6) {
            t->dragging = true;
        }
        if (t->dragging && ui_contains(bar, c->x, c->y)) {
            int target = i;
            for (int k = i + 1; k < t->count; k++) {
                UiRect neighbor = ui_tab_rect(t, bar, width, k);
                if (c->x > neighbor.x + neighbor.w * .5f) {
                    target = k;
                }
            }
            for (int k = i - 1; k >= 0; k--) {
                UiRect neighbor = ui_tab_rect(t, bar, width, k);
                if (c->x < neighbor.x + neighbor.w * .5f) {
                    target = k;
                }
            }
            if (target < 0) {
                target = 0;
            }
            if (target >= t->count) {
                target = t->count - 1;
            }
            if (target != i) {
                UiTab moving = t->items[i];
                if (target > i) {
                    memmove(t->items + i, t->items + i + 1, (size_t)(target - i) * sizeof(UiTab));
                } else {
                    memmove(t->items + target + 1, t->items + target,
                            (size_t)(i - target) * sizeof(UiTab));
                }
                t->items[target] = moving;
                i = target;
            }
        }
        if (c->released) {
            if (t->dragging || ui_contains(bar, c->release_x, c->release_y)) {
                t->selected = tab.id;
            }
            c->active = 0;
            t->dragging = false;
        }
        break;
    }

    return before != t->selected;
}

UiSplit ui_split_rects(UiRect area, UiAxis axis, float second_size, float first_min,
                       float second_min, float gap) {
    if (!isfinite(area.w) || !isfinite(area.h) || area.w < 0 || area.h < 0) {
        area.w = area.h = 0;
    }
    float total = axis == UI_AXIS_Y ? area.h : area.w;
    gap = isfinite(gap) ? clamp(gap, 0, total) : 0;
    float available = total - gap;
    first_min = isfinite(first_min) ? fmaxf(0, first_min) : 0;
    second_min = isfinite(second_min) ? fmaxf(0, second_min) : 0;
    if (first_min + second_min > available) {
        float ratio = available / (first_min + second_min);
        first_min *= ratio;
        second_min *= ratio;
    }
    second_size = clamp(isfinite(second_size) ? second_size : second_min, second_min,
                        fmaxf(second_min, available - first_min));
    float first_size = fmaxf(0, available - second_size);
    UiSplit result = {area, area, area, second_size};
    if (axis == UI_AXIS_Y) {
        result.first.h = first_size;
        result.grip.y += first_size;
        result.grip.h = gap;
        result.second.y += first_size + gap;
        result.second.h = second_size;
    } else {
        result.first.w = first_size;
        result.grip.x += first_size;
        result.grip.w = gap;
        result.second.x += first_size + gap;
        result.second.w = second_size;
    }
    return result;
}

UiPropertyRow ui_property_row(UiRect area, float label_fraction, float gap) {
    area.w = fmaxf(0, area.w);
    area.h = fmaxf(0, area.h);
    gap = fminf(area.w, fmaxf(0, gap));
    float left = (area.w - gap) * fminf(1, fmaxf(0, label_fraction));
    return (UiPropertyRow){{area.x, area.y, left, area.h},
                           {area.x + left + gap, area.y, area.w - left - gap, area.h}};
}

bool ui_tree_continues(const int *depths, int count, int row, int level) {
    if (!depths || row < 0 || row >= count || level < 0 || level > depths[row]) {
        return false;
    }
    for (int i = row + 1; i < count; i++) {
        if (depths[i] < level) {
            return false;
        }
        if (depths[i] == level) {
            return true;
        }
    }
    return false;
}
