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
    app.inspector_view.offset = 0;
    app.map->component_add_open = false;
    app.map->reveal_selected = true;
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
    if (add) {
        app.map->reveal_selected = true;
    }
    app.map->component_closed[entity.index] &= ~bit;
    app.map->component_add_open = false;
    return true;
}

static UiRect property_value(float x, float y, float w, const char *name) {
    UiPropertyRow row = ui_property_row((UiRect){x, y, w, 25}, .45f, 8);
    ui_clip_push(&app.ui, row.label);
    ui_apply_clip(app.renderer, &app.ui);
    editor_text(row.label.x, row.label.y + 10, name, UI_MUTED);
    ui_clip_pop(&app.ui);
    ui_apply_clip(app.renderer, &app.ui);
    return row.value;
}

static void property_field(float x, float y, float w, const char *name, const char *value,
                           const char *input, Field field, bool enabled) {
    UiRect cell = property_value(x, y, w, name);
    UiId id = ui_id("component_input") ^ app.selected * 31u ^ (UiId)field * 16777619u;
    if (!id) {
        id = 1;
    }
    editor_register_ui_hit(cell);
    bool focused = app.field_inline && app.field == field && app.field_entity == app.selected;
    if (!focused && ui_button(&app.ui, id, cell, enabled)) {
        editor_field_start_inline(field, input, id, cell);
        focused = true;
    }
    UiTextEdit idle = {0};
    if (focused) {
        app.field_seen = true;
        app.field_rect = cell;
    }
    ui_draw_text_input(app.renderer, (SDL_FRect){cell.x, cell.y, cell.w, cell.h},
                       focused ? app.input : value, focused ? &app.field_edit : &idle, focused,
                       enabled, focused && app.field_invalid,
                       (SDL_GetTicks() - app.field_blink) % 1000 < 500);
}

static void number_field(float x, float y, float w, const char *name, float value, Field field,
                         bool enabled) {
    char input[64];
    snprintf(input, sizeof input, "%.2f", value);
    property_field(x, y, w, name, input, input, field, enabled);
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
                                        : 2 * 29.f;
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
                snprintf(label, sizeof label, "#%u (local)", o->parent_id);
                UiRect cell = property_value(fx, fy, fw, "Parent");
                editor_text(cell.x + 9, cell.y + 10, label, UI_MUTED);
                fy += 22;
            }
            number_field(fx, fy, fw, o->parent_id ? "Local X" : "X", o->x, FIELD_X, enabled);
            number_field(fx, fy + 29, fw, o->parent_id ? "Local Y" : "Y", o->y, FIELD_Y, enabled);
            number_field(fx, fy + 58, fw, "Width", o->w, FIELD_W, enabled);
            number_field(fx, fy + 87, fw, "Height", o->h, FIELD_H, enabled);
        } else if (bit == PE_SPRITE) {
            char label[64], color[16];
            snprintf(label, sizeof label, "#%08X", (unsigned)o->color);
            snprintf(color, sizeof color, "%08X", (unsigned)o->color);
            property_field(fx, fy, fw, "Color", label, color, FIELD_COLOR, enabled);
        } else if (bit == PE_BODY) {
            number_field(fx, fy, fw, "Velocity X", o->vx, FIELD_VX, enabled);
            number_field(fx, fy + 29, fw, "Velocity Y", o->vy, FIELD_VY, enabled);
        } else {
            UiRect cell = property_value(fx, fy, fw, "Controls");
            editor_text(cell.x + 9, cell.y + 10, "WASD/arrows", UI_TEXT);
            cell = property_value(fx, fy + 29, fw, "Speed");
            editor_text(cell.x + 9, cell.y + 10, "90 px/s", UI_TEXT);
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
    float button_width = fminf(width, ui_tab_content_width(ICON_PLUS, "Add component") + 20);
    float button_x = x + width - button_width;
    app.map->component_anchor = (SDL_FRect){button_x, y, button_width, 25};
    app.map->component_anchor_seen = true;
    if (icon_button(button_x, y, button_width, ICON_PLUS, "Add component", "Choose a component",
                    app.map->component_add_open, enabled)) {
        app.map->component_add_open = !app.map->component_add_open;
        app.map->component_owner = o->stable_id;
        app.map->component_pressed = -1;
        app.map->component_choice = -1;
        for (int i = 0; i < 4; i++) {
            if (!(o->mask & components[i].bit)) {
                app.map->component_choice = i;
                break;
            }
        }
        if (app.map->component_add_open) {
            app.ui.blocked = true;
        }
    }
    ui_icon(app.renderer, app.map->component_add_open ? ICON_UP : ICON_DOWN, x + width - 20, y + 6,
            12, enabled ? UI_TEXT : UI_DISABLED);
    y += 33;
    for (int i = 0; i < 4; i++) {
        if (o->mask & components[i].bit) {
            y = component_card(entity, i, x, y, width, enabled);
        }
    }
    app.widget_scope = 0;
    return y;
}

static void close_popup(void) {
    app.map->component_add_open = false;
    app.map->component_pressed = -1;
    app.needs_layout = true;
    ui_cancel(&app.ui);
}

void editor_draw_component_popup(void) {
    if (!app.map || !app.map->component_add_open) {
        return;
    }
    PeObject *o = pe_entity_get(&app.map->scene, pe_entity_find(&app.map->scene, app.selected));
    if (!o || o->stable_id != app.map->component_owner || !app.map->component_anchor_seen ||
        app.playing || app.window_collapsed || app.field != FIELD_NONE ||
        app.request != REQUEST_NONE) {
        close_popup();
        return;
    }
    SDL_FRect anchor = app.map->component_anchor;
    SDL_FRect popup = {fmaxf(4, fminf(anchor.x, app.width - anchor.w - 4)), anchor.y + anchor.h + 4,
                       anchor.w, 120};
    if (popup.y + popup.h > app.height - BOTTOM - 4) {
        popup.y = anchor.y - popup.h - 4;
    }
    popup.y = fmaxf(TOP + 4, popup.y);
    app.map->component_popup = popup;
    editor_box((SDL_FRect){popup.x + 3, popup.y + 3, popup.w, popup.h}, UI_CANVAS);
    editor_box(popup, UI_BORDER);
    editor_box((SDL_FRect){popup.x + 1, popup.y + 1, popup.w - 2, popup.h - 2},
               editor_widget_background());
    for (int i = 0; i < 4; i++) {
        bool present = (o->mask & components[i].bit) != 0;
        SDL_FRect row = {popup.x + 4, popup.y + 4 + i * 28, popup.w - 8, 28};
        if (!present && app.map->component_choice == i) {
            editor_box(row, UI_HOVER);
        }
        ui_icon(app.renderer, components[i].icon, row.x + 7, row.y + 6, 16,
                present ? UI_DISABLED : UI_ACCENT);
        editor_text(row.x + 31, row.y + 10, components[i].name, present ? UI_MUTED : UI_TEXT);
        if (present) {
            ui_icon(app.renderer, ICON_CHECK, row.x + row.w - 23, row.y + 6, 16, UI_MUTED);
        }
    }
}

bool editor_component_popup_event(const SDL_Event *e) {
    if (!app.map || !app.map->component_add_open) {
        return false;
    }
    if (e->type == SDL_EVENT_QUIT || e->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED ||
        e->type == SDL_EVENT_WINDOW_FOCUS_LOST || e->type == SDL_EVENT_WINDOW_RESIZED) {
        close_popup();
        return false;
    }
    PeObject *o = pe_entity_get(&app.map->scene, pe_entity_find(&app.map->scene, app.selected));
    if (!o || o->stable_id != app.map->component_owner) {
        close_popup();
        return false;
    }
    if (e->type == SDL_EVENT_KEY_DOWN) {
        if (e->key.repeat) {
            return true;
        }
        if (e->key.key == SDLK_ESCAPE) {
            close_popup();
        } else if (e->key.key == SDLK_DOWN || e->key.key == SDLK_UP) {
            int direction = e->key.key == SDLK_DOWN ? 1 : -1;
            int choice = app.map->component_choice;
            if (choice < 0) {
                choice = direction > 0 ? 3 : 0;
            }
            for (int i = 0; i < 4; i++) {
                choice = (choice + direction + 4) % 4;
                if (!(o->mask & components[choice].bit)) {
                    app.map->component_choice = choice;
                    break;
                }
            }
        } else if (e->key.key == SDLK_RETURN || e->key.key == SDLK_SPACE) {
            int choice = app.map->component_choice;
            if (choice >= 0 && choice < 4 && !(o->mask & components[choice].bit)) {
                editor_component_change(components[choice].bit, true);
                close_popup();
            }
        }
        return true;
    }
    if (e->type == SDL_EVENT_MOUSE_MOTION || e->type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
        e->type == SDL_EVENT_MOUSE_BUTTON_UP) {
        float x = e->type == SDL_EVENT_MOUSE_MOTION ? e->motion.x : e->button.x;
        float y = e->type == SDL_EVENT_MOUSE_MOTION ? e->motion.y : e->button.y;
        SDL_FRect p = app.map->component_popup;
        int row = editor_inside(x, y, p) ? (int)floorf((y - p.y - 4) / 28) : -1;
        if (row < 0 || row >= 4 || (o->mask & components[row].bit)) {
            row = -1;
        }
        if (row >= 0) {
            app.map->component_choice = row;
        }
        if (e->type == SDL_EVENT_MOUSE_BUTTON_DOWN && e->button.button == SDL_BUTTON_LEFT) {
            if (!editor_inside(x, y, p)) {
                close_popup();
            } else {
                app.map->component_pressed = row;
            }
        } else if (e->type == SDL_EVENT_MOUSE_BUTTON_UP && e->button.button == SDL_BUTTON_LEFT) {
            if (row >= 0 && row == app.map->component_pressed) {
                editor_component_change(components[row].bit, true);
                close_popup();
            } else {
                app.map->component_pressed = -1;
            }
        }
        return true;
    }
    return e->type == SDL_EVENT_TEXT_INPUT || e->type == SDL_EVENT_MOUSE_WHEEL ||
           e->type == SDL_EVENT_KEY_UP;
}
