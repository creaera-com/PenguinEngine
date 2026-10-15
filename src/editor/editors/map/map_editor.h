#ifndef PENGUIN_MAP_EDITOR_H
#define PENGUIN_MAP_EDITOR_H

#include "penguin/render.h"

typedef enum { BRUSH, ERASE, RECTANGLE, FILL, SELECT } Tool;

typedef struct {
    PeScene scene, runtime;
    PeHistory history;
    PeAssets assets;
} MapPayload;

/* Map-specific operations, used only within the editor. */
void editor_assets_sync(void);
bool editor_begin(void);
void editor_commit(void);
void editor_draw_info(void);
void editor_draw_inspector(void);
void editor_draw_map_content(void);
void editor_draw_map_tools(SDL_FRect area);
void editor_draw_scene_tree(void);
void editor_edit_end(bool erase);
void editor_edit_start(float sx, float sy, bool erase);
void editor_field_apply(void);
bool editor_import_png(const char *source);
void editor_load_scene(const char *path);
void editor_paint_line(int x0, int y0, int x1, int y1, bool erase);
void editor_play_toggle(void);
bool editor_save_scene(void);
void editor_selection_sync(void);
bool editor_set_tileset(const char *relative, int tile_size);
void editor_tile_coords(float sx, float sy, int *x, int *y);
void editor_map_key(SDL_Keycode key);
void editor_map_zoom(float delta);
void editor_map_pointer(SDL_Event *event, bool modal);
void editor_map_action(const char *name);
void editor_map_update(double elapsed);

#endif
