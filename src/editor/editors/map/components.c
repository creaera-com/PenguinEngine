#include "app/editor_internal.h"

static const struct {
    uint32_t bit;
    const char *name;
    UiIcon icon;
} components[] = {{PE_TRANSFORM, "Transform", ICON_SELECT},
                  {PE_SPRITE, "Sprite", ICON_IMAGE},
                  {PE_BODY, "Body", ICON_COLLISION},
                  {PE_PLAYER, "Player", ICON_ENTITY}};

void editor_inspect_entity(void) {
    app.right_tabs.selected = UI_INSPECTOR_TAB;
    app.entity_open = true;
    app.layers_open = app.palette_open = false;
    app.inspector_view.offset = 0;
    app.map->component_add_open = false;
}

bool editor_component_change(uint32_t bit, bool add) {
    if (!editor_map_active() || app.playing || editor_edit_pending() || app.field != FIELD_NONE ||
        app.request != REQUEST_NONE) {
        return false;
    }
    PeEntity entity = pe_entity_find(&app.map->scene, app.selected);
    if (!pe_entity_valid(&app.map->scene, entity) || !editor_begin()) {
        return false;
    }
    bool changed = add ? pe_entity_add_component(&app.map->scene, entity, bit)
                       : pe_entity_remove_component(&app.map->scene, entity, bit);
    if (!changed) {
        pe_history_cancel(&app.map->history, &app.map->scene);
        return false;
    }
    uint64_t revision = app.map->history.revision;
    editor_commit();
    if (app.map->history.revision == revision) {
        return false;
    }
    app.map->component_closed[entity.index] &= ~bit;
    app.map->component_add_open = false;
    return true;
}

static void number_field(float x, float y, float w, const char *name, float value, Field field,
                         bool enabled) {
    char label[80], input[64];
    snprintf(label, sizeof label, "%s: %.2f", name, value);
    app.widget_scope ^= (UiId)field * 16777619u;
    if (button(x, y, w, label, false, enabled)) {
        snprintf(input, sizeof input, "%.2f", value);
        editor_field_start(field, input);
    }
    app.widget_scope ^= (UiId)field * 16777619u;
}

static float component_card(PeEntity entity, int index, float x, float y, float w, bool enabled) {
    PeObject *o = pe_entity_get(&app.map->scene, entity);
    uint32_t bit = components[index].bit;
    app.widget_scope = ui_id("component") ^ o->stable_id * 31u ^ bit;
    bool open = !(app.map->component_closed[entity.index] & bit);
    UiRect header = {x + 4, y + 4, fmaxf(0, w - 40), 25};
    ui_disclosure(&app.ui, app.widget_scope, header, &open, !editor_edit_pending());
    if (open) {
        app.map->component_closed[entity.index] &= ~bit;
    } else {
        app.map->component_closed[entity.index] |= bit;
    }
    float content = bit == PE_TRANSFORM ? 4 * 29.f + (o->parent_id ? 22 : 0)
                    : bit == PE_BODY    ? 2 * 29.f
                    : bit == PE_SPRITE  ? 29
                                        : 24;
    float height = open ? 34 + content + 8 : 33;
    editor_box((SDL_FRect){x, y, w, height}, editor_widget_background());
    if (ui_hovered(&app.ui, header)) {
        editor_box((SDL_FRect){header.x, header.y, header.w, header.h}, UI_HOVER);
    }
    ui_icon(app.renderer, open ? ICON_DOWN : ICON_RIGHT, x + 9, y + 10, 12, UI_MUTED);
    ui_icon(app.renderer, components[index].icon, x + 27, y + 8, 16, UI_ACCENT);
    editor_text(x + 51, y + 12, components[index].name, UI_TEXT);
    bool removable = pe_entity_component_removable(&app.map->scene, entity, bit);
    const char *hint = removable        ? "Remove component"
                       : bit == PE_BODY ? "Remove Player first"
                                        : "Required by components or hierarchy";
    if (icon_button(x + w - 32, y + 4, 28, ICON_CLOSE, "", hint, false, enabled && removable)) {
        editor_component_change(bit, false);
        return y + height + 6;
    }
    if (open) {
        float fy = y + 34, fx = x + 8, fw = w - 16;
        if (bit == PE_TRANSFORM) {
            if (o->parent_id) {
                char label[64];
                snprintf(label, sizeof label, "Parent: #%u (local)", o->parent_id);
                editor_text(fx, fy + 8, label, UI_MUTED);
                fy += 22;
            }
            number_field(fx, fy, fw, o->parent_id ? "Local X" : "X", o->x, FIELD_X, enabled);
            number_field(fx, fy + 29, fw, o->parent_id ? "Local Y" : "Y", o->y, FIELD_Y, enabled);
            number_field(fx, fy + 58, fw, "Width", o->w, FIELD_W, enabled);
            number_field(fx, fy + 87, fw, "Height", o->h, FIELD_H, enabled);
        } else if (bit == PE_SPRITE) {
            char label[64], color[16];
            snprintf(label, sizeof label, "Color: #%08X", (unsigned)o->color);
            if (button(fx, fy, fw, label, false, enabled)) {
                snprintf(color, sizeof color, "%08X", (unsigned)o->color);
                editor_field_start(FIELD_COLOR, color);
            }
        } else if (bit == PE_BODY) {
            number_field(fx, fy, fw, "Velocity X", o->vx, FIELD_VX, enabled);
            number_field(fx, fy + 29, fw, "Velocity Y", o->vy, FIELD_VY, enabled);
        } else {
            editor_text(fx, fy + 8, "WASD / arrows | 90 px/s", UI_MUTED);
        }
    }
    return y + height + 6;
}

float editor_draw_components(float x, float y, float width) {
    PeEntity entity = pe_entity_find(&app.map->scene, app.selected);
    PeObject *o = pe_entity_get(&app.map->scene, entity);
    if (!o) {
        return y;
    }
    if (app.map->component_ids[entity.index] != o->stable_id) {
        app.map->component_ids[entity.index] = o->stable_id;
        app.map->component_closed[entity.index] = 0;
        app.map->component_add_open = false;
    }
    bool enabled = !app.playing && !editor_edit_pending();
    if ((o->mask & 15u) == 15u) {
        app.map->component_add_open = false;
    }
    if (icon_button(x, y, width, ICON_PLUS, "Add component", "Show missing components", false,
                    enabled && (o->mask & 15u) != 15u)) {
        app.map->component_add_open = !app.map->component_add_open;
    }
    y += 33;
    if (app.map->component_add_open && enabled) {
        for (int i = 0; i < 4; i++) {
            if (o->mask & components[i].bit) {
                continue;
            }
            app.widget_scope = ui_id("add_component") ^ o->stable_id ^ components[i].bit * 31u;
            if (icon_button(x + 8, y, width - 16, components[i].icon, components[i].name,
                            "Add component and required dependencies", false, true)) {
                editor_component_change(components[i].bit, true);
            }
            y += 29;
        }
        y += 4;
    }
    for (int i = 0; i < 4; i++) {
        if (o->mask & components[i].bit) {
            y = component_card(entity, i, x, y, width, enabled);
        }
    }
    app.widget_scope = 0;
    return y;
}
