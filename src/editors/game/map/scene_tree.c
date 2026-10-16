#include "editors/game/map/internal.h"

typedef struct {
    PeEntity entity;
    int depth;
    bool children;
} SceneRow;

static bool collapsed(uint32_t id) {
    for (int i = 0; i < app.workspace.map->collapsed_count; i++) {
        if (app.workspace.map->collapsed[i] == id) {
            return true;
        }
    }
    return false;
}

static void expand(uint32_t id) {
    for (int i = 0; i < app.workspace.map->collapsed_count; i++) {
        if (app.workspace.map->collapsed[i] == id) {
            memmove(&app.workspace.map->collapsed[i], &app.workspace.map->collapsed[i + 1],
                    (size_t)(app.workspace.map->collapsed_count - i - 1) * sizeof(uint32_t));
            app.workspace.map->collapsed_count--;
            return;
        }
    }
}

static void collect(SceneRow *rows, int *count, uint32_t parent, int depth) {
    if (depth >= PE_ENTITIES) {
        return;
    }
    for (uint32_t i = 0; i < PE_ENTITIES && *count < PE_ENTITIES; i++) {
        const PeObject *o = &app.workspace.map->scene.objects[i];
        if (!o->alive || o->parent_id != parent) {
            continue;
        }
        bool children = false;
        for (int j = 0; j < PE_ENTITIES; j++) {
            if (app.workspace.map->scene.objects[j].alive &&
                app.workspace.map->scene.objects[j].parent_id == o->stable_id) {
                children = true;
                break;
            }
        }
        rows[(*count)++] = (SceneRow){{i, o->generation}, depth, children};
        if (children && !collapsed(o->stable_id)) {
            collect(rows, count, o->stable_id, depth + 1);
        }
    }
}

void editor_scene_add(int relation) {
    if (!editor_map_active() || app.workspace.map->view.playing || editor_edit_pending() ||
        relation < 0 || relation > 2) {
        return;
    }
    PeScene *s = &app.workspace.map->scene;
    PeObject *selected = pe_entity_get(s, pe_entity_find(s, app.shell.selected));
    if (relation && !selected) {
        return;
    }
    uint32_t parent_id = relation == 2   ? selected->stable_id
                         : relation == 1 ? selected->parent_id
                                         : 0;
    float x = relation == 1 ? selected->x + 16 : relation == 2 ? 16 : 64;
    float y = relation == 1 ? selected->y + 16 : relation == 2 ? 16 : 64;
    if (!editor_begin()) {
        return;
    }
    PeEntity entity = pe_entity_create(s, PE_TRANSFORM | PE_SPRITE | PE_BODY);
    PeObject *o = pe_entity_get(s, entity);
    if (!o) {
        pe_history_cancel(&app.workspace.map->history, s);
        editor_message("Entity limit reached");
        return;
    }
    o->x = x;
    o->y = y;
    if (parent_id && !pe_entity_set_parent(s, entity, pe_entity_find(s, parent_id), false)) {
        pe_history_cancel(&app.workspace.map->history, s);
        editor_message("Cannot create child: parent requires a valid transform");
        return;
    }
    uint32_t id = o->stable_id;
    editor_commit();
    if (pe_entity_valid(s, pe_entity_find(s, id))) {
        app.shell.selected = id;
        app.workspace.map->view.tool = SELECT;
        app.shell.left_tabs.selected = UI_SCENE_TAB;
        editor_inspect_entity();
        PeEntity ancestor = pe_entity_find(s, parent_id);
        for (int depth = 0; pe_entity_valid(s, ancestor) && depth < PE_ENTITIES; depth++) {
            const PeObject *parent = pe_entity_get(s, ancestor);
            expand(parent->stable_id);
            ancestor = pe_entity_find(s, parent->parent_id);
        }
    }
}

void editor_scene_delete(void) {
    if (!editor_map_active() || app.workspace.map->view.playing || editor_edit_pending()) {
        return;
    }
    PeEntity entity = pe_entity_find(&app.workspace.map->scene, app.shell.selected);
    PeObject *o = pe_entity_get(&app.workspace.map->scene, entity);
    if (o && editor_begin()) {
        uint32_t parent = o->parent_id;
        pe_entity_destroy(&app.workspace.map->scene, entity);
        editor_commit();
        if (!pe_entity_valid(&app.workspace.map->scene,
                             pe_entity_find(&app.workspace.map->scene, app.shell.selected))) {
            app.shell.selected = parent;
        }
    }
}

static bool row_button(UiId id, UiRect rect, UiIcon icon, const char *label, const char *hint,
                       bool selected, bool enabled, Uint32 background) {
    UiTheme theme = ui_theme;
    theme.surface = theme.panel = background;
    bool hover = ui_hovered(&app.shell.ui, rect);
    editor_register_ui_hit(rect);
    ui_draw_button_themed(app.platform.renderer, &theme,
                          (SDL_FRect){rect.x, rect.y, rect.w, rect.h}, icon, label, selected,
                          enabled, hover);
    if (hover) {
        editor_tooltip = hint;
    }
    return ui_button(&app.shell.ui, id, rect, enabled);
}

void editor_draw_scene_tree(void) {
    /* Drop obsolete navigation state after deletion/replacing a document. */
    for (int i = app.workspace.map->collapsed_count - 1; i >= 0; i--) {
        if (!pe_entity_valid(
                &app.workspace.map->scene,
                pe_entity_find(&app.workspace.map->scene, app.workspace.map->collapsed[i]))) {
            expand(app.workspace.map->collapsed[i]);
        }
    }
    SceneRow rows[PE_ENTITIES];
    int count = 0;
    collect(rows, &count, 0, 0);
    int depths[PE_ENTITIES];
    for (int i = 0; i < count; i++) {
        depths[i] = rows[i].depth;
    }
    SDL_FRect body = {app.shell.panel_area.x + 6, app.shell.panel_area.y + 6,
                      fmaxf(0, app.shell.panel_area.w - 12), fmaxf(0, app.shell.panel_area.h - 12)};
    editor_box(body, editor_widget_background());
    app.shell.scene_view.content = count ? count * 26.f : 26;
    app.shell.scene_view.viewport = fmaxf(0, body.h - 8);
    ui_scroll_clamp(&app.shell.scene_view);
    UiRect track = {body.x + body.w - 12, body.y + 4, 8, app.shell.scene_view.viewport};
    ui_scrollbar(&app.shell.ui, UI_SCENE_SCROLL, track, &app.shell.scene_view, UI_AXIS_Y);
    ui_clip_push(&app.shell.ui, (UiRect){body.x + 4, body.y + 4, fmaxf(0, body.w - 20),
                                         app.shell.scene_view.viewport});
    ui_apply_clip(app.platform.renderer, &app.shell.ui);
    if (!count) {
        editor_text(body.x + 8, body.y + 12, "No entities in scene.", UI_MUTED);
    }
    bool enabled = !app.workspace.map->view.playing && !editor_edit_pending();
    for (int i = 0; i < count; i++) {
        const PeObject *o = pe_entity_get(&app.workspace.map->scene, rows[i].entity);
        float y = body.y + 4 + i * 26 - app.shell.scene_view.offset;
        Uint32 background = editor_tree_row_background(i);
        editor_box((SDL_FRect){body.x + 4, y, fmaxf(0, body.w - 20), 26}, background);
        float indent = fminf(rows[i].depth * 20.f, fmaxf(0, body.w - 100));
        float x = body.x + 4 + indent;
        app.shell.widget_scope = ui_id("scene_entity") ^ o->stable_id;
        bool open = !collapsed(o->stable_id);
        if (rows[i].children &&
            row_button(app.shell.widget_scope ^ ui_id("scene_expand"), (UiRect){x, y, 20, 25},
                       open ? ICON_DOWN : ICON_RIGHT, "", "Expand / collapse", false,
                       !editor_edit_pending(), background)) {
            if (open) {
                app.workspace.map->collapsed[app.workspace.map->collapsed_count++] = o->stable_id;
            } else {
                expand(o->stable_id);
            }
        }
        const char *label = o->name;
        if (row_button(app.shell.widget_scope ^ ui_id("scene_select"),
                       (UiRect){x + 22, y, fmaxf(0, body.w - indent - 42), 25}, ICON_ENTITY, label,
                       "Select entity", app.shell.selected == o->stable_id, enabled, background)) {
            app.shell.selected = o->stable_id;
            app.workspace.map->view.tool = SELECT;
            editor_inspect_entity();
        }
        ui_draw_tree_guides(app.platform.renderer, depths, count, i,
                            (UiRect){body.x + 4, y, fmaxf(0, body.w - 20), 26}, 20, 10,
                            fmaxf(0, body.w - 100), x + (rows[i].children ? -2 : 28), UI_DISABLED);
    }
    app.shell.widget_scope = 0;
    ui_clip_pop(&app.shell.ui);
    ui_apply_clip(app.platform.renderer, &app.shell.ui);
    ui_draw_scrollbar(app.platform.renderer, &app.shell.ui, UI_SCENE_SCROLL, track,
                      &app.shell.scene_view, UI_AXIS_Y);
}
