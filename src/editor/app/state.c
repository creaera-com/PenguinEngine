#include "app/editor_internal.h"
Editor app;
const EditorView editor_views[] = {
    {UI_MAP_VIEW, "Map", ICON_GRID, 9, editor_draw_map_tools, editor_draw_map_content},
    {UI_SCRIPT_VIEW, "Scripts", ICON_FILE, 1, editor_draw_script_tools,
     editor_draw_script_content}};
const int editor_view_count = (int)(sizeof editor_views / sizeof editor_views[0]);

bool editor_map_active(void) {
    return app.loaded_view == UI_MAP_VIEW;
}

bool editor_scripts_active(void) {
    return app.loaded_view == UI_SCRIPT_VIEW;
}

bool editor_edit_pending(void) {
    return app.map && app.map->history.pending != NULL;
}

bool editor_map_dirty(void) {
    return app.map ? pe_history_dirty(&app.map->history) : app.session_dirty[0];
}

bool editor_script_dirty(void) {
    return app.script ? se_dirty(app.script) : app.session_dirty[1];
}

const EditorView *editor_view(UiId id) {
    for (int i = 0; i < editor_view_count; i++)
        if (editor_views[i].id == id)
            return &editor_views[i];
    return &editor_views[0];
}

void editor_message(const char *s) {
    char copy[256] = {0};
    snprintf(copy, sizeof copy, "%s", s);
    memcpy(app.status, copy, sizeof copy);
    if (app.script)
        memcpy(app.script->status, copy, sizeof copy);
}
