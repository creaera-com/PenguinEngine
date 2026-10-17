/** @file @brief SDL-free tab selection, reordering, geometry and scrolling. */
#include "core/internal.h"
#include <string.h>

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
        if (ui_context_acquire(c, tab.id, r)) {
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
