#ifndef PENGUIN_EDITOR_INTERNAL_H
#define PENGUIN_EDITOR_INTERNAL_H

#include "app/shell.h"
#include "app/ui_helpers.h"
#include "browser/file_tree.h"
#include "dialogs/fields.h"
#include "editors/map/checkpoint.h"
#include "editors/map/map_editor.h"
#include "editors/scripts/checkpoint.h"
#include "editors/scripts/panels.h"
#include "editors/scripts/script_editor.h"
#include "layout/runtime.h"
#include "layout/ui_layout.h"
#include "penguin/render.h"
#include "penguin/ui_sdl.h"
#include "workspace/session.h"
#include "workspace/workspace.h"
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define LEFT ((int)app.left_width)
#define RIGHT ((int)app.right_width)
#define TOP 50
#define TABS 32
#define BOTTOM 28
#define PANEL_GAP (app.gap)
#define GLOBAL_NAV_X 232
#define SIDE_TOP (TOP + TABS)
#define TREE_MAX 256
#define UI_SPLIT_LEFT 0x10001u
#define UI_SPLIT_RIGHT 0x10002u
#define UI_TREE_SCROLL 0x10003u
#define UI_INSPECTOR_SCROLL 0x10004u
#define UI_SCRIPT_VSCROLL 0x10005u
#define UI_SCRIPT_HSCROLL 0x10006u
#define UI_SCRIPT_DOCUMENT 0x10007u
#define UI_MAP_VIEW 0x10008u
#define UI_SCRIPT_VIEW 0x10009u
#define UI_EXPLORER_TAB 0x1000au
#define UI_SCENE_TAB 0x1000bu
#define UI_INSPECTOR_TAB 0x1000cu
#define UI_INFO_TAB 0x1000du
#define UI_SCENE_SCROLL 0x1000eu
#define UI_INFO_SCROLL 0x1000fu
typedef struct {
    UiId id, selected, order[UI_MAX_TABS];
    int count, open, dragging;
    float scroll;
} WidgetState;
typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Cursor *resize_cursor;
    UiContext ui;
    UiId widget_scope;
    UiTabs workspaces, left_tabs, right_tabs;
    UiScroll tree_view, inspector_view, scene_view, info_view;
    float left_width, right_width;
    bool layers_open, entity_open, palette_open;
    MapPayload *map;
    PeScriptEditor *script;
    UiId loaded_view;
    char session_paths[2][1200];
    bool session_exists[2], session_dirty[2];
    PeUiLayout *shell_layout, *active_layout;
    char ui_path[1024];
    int gap, tab_gap;
    float min_left, min_right, min_center;
    SDL_FRect panel_area;
    float panel_top;
    int global_nav_x;
    WidgetState widgets[64];
    int widget_count;
    UiRect hits[PE_UI_MAX_NODES];
    int hit_count;
    float wheel_delta;
    bool canvas_enabled, rendering, deferred_save, needs_layout;
    UiId pending_view;
    Request deferred_request;
    char deferred_path[1024];
    PeCamera camera;
    char root[768], scene_path[1024], status[256];
    int width, height, layer, tile, tool, palette_scroll, tree_scroll;
    bool playing, grid, collision, running, smoke, painting, panning, dragging;
    int start_x, start_y, last_x, last_y;
    float drag_x, drag_y;
    uint32_t selected;
    float mx, my;
    bool click;
    SDL_FRect viewport;
    Field field;
    char input[1024];
    Request request;
    char open_path[1024];
    TreeRow tree[TREE_MAX];
    int tree_count;
    char expanded[64][PE_PATH];
    int expanded_count;
    double accumulator;
    Uint64 last_time;
} Editor;
typedef struct {
    UiId id;
    const char *label;
    UiIcon icon;
    int action_count;
    void (*draw_tools)(SDL_FRect area);
    void (*draw_content)(void);
} EditorView;

/* Private integration state. Engine and reusable UI never include this header. */
extern Editor app;
extern const EditorView editor_views[];
extern const int editor_view_count;
extern const char *editor_tooltip;
extern const char *editor_screenshot_path;
bool editor_edit_pending(void);
const EditorView *editor_view(UiId id);
bool editor_map_active(void);
bool editor_map_dirty(void);
void editor_message(const char *s);
bool editor_script_dirty(void);
bool editor_scripts_active(void);

/* File identity prevents equal line numbers in different modules sharing IDs. */
#define icon_button(...) editor_icon_button_impl(ui_id(__FILE__) ^ (UiId)__LINE__, __VA_ARGS__)
#define button(...) editor_button_impl(ui_id(__FILE__) ^ (UiId)__LINE__, __VA_ARGS__)

#endif
