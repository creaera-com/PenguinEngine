#include "editors/game/map/internal.h"

bool editor_begin(void) {
    if (!pe_history_begin(&app.workspace.map->history, &app.workspace.map->scene)) {
        editor_message("Cannot begin edit (pending edit or out of memory)");
        return false;
    }

    return true;
}

void editor_commit(void) {
    if (!pe_history_commit(&app.workspace.map->history, &app.workspace.map->scene)) {
        editor_message("Out of memory: edit reverted");
    }
}

void editor_assets_sync(void) {
    char error[256];
    if (!pe_assets_load(&app.workspace.map->assets, app.platform.renderer, app.project.root,
                        &app.workspace.map->scene, error, sizeof error)) {
        editor_message(error);
    }
}

void editor_selection_sync(void) {
    if (app.workspace.map->view.layer >= app.workspace.map->scene.layer_count) {
        app.workspace.map->view.layer = app.workspace.map->scene.layer_count - 1;
    }
    if (!pe_entity_valid(&app.workspace.map->scene,
                         pe_entity_find(&app.workspace.map->scene, app.shell.selected))) {
        app.shell.selected = 0;
    }
    editor_assets_sync();
}

bool editor_save_scene(void) {
    char error[256];
    if (!pe_scene_save(&app.workspace.map->scene, app.project.scene_path, error, sizeof error)) {
        editor_message(error);
        return false;
    }
    pe_history_saved(&app.workspace.map->history);
    editor_message("Scene saved");
    return true;
}

void editor_load_scene(const char *path) {
    PeScene *next = malloc(sizeof *next);
    char error[256];
    if (!next) {
        editor_message("Out of memory");
        return;
    }
    if (!pe_scene_load(next, path, error, sizeof error)) {
        editor_message(error);
        free(next);
        return;
    }
    app.workspace.map->scene = *next;
    free(next);
    snprintf(app.project.scene_path, sizeof app.project.scene_path, "%s", path);
    pe_history_free(&app.workspace.map->history);
    app.workspace.map->view.layer = 0;
    app.shell.selected = 0;
    app.workspace.map->collapsed_count = 0;
    memset(app.workspace.map->component_ids, 0, sizeof app.workspace.map->component_ids);
    memset(app.workspace.map->component_closed, 0, sizeof app.workspace.map->component_closed);
    app.workspace.map->component_add_open = false;
    app.workspace.map->view.camera = (PeCamera){-16, -16, 2};
    editor_assets_sync();
    editor_tab_select(false);
    editor_message("Scene opened");
}

bool editor_set_tileset(const char *relative, int tile_size) {
    PeScene candidate = app.workspace.map->scene;
    snprintf(candidate.tileset, sizeof candidate.tileset, "%s", relative);
    candidate.tileset_tile_size = tile_size;
    PeAssets check = {0};
    char error[256];
    if (!pe_assets_load(&check, app.platform.renderer, app.project.root, &candidate, error,
                        sizeof error)) {
        editor_message(error);
        return false;
    }
    if (!editor_begin()) {
        pe_assets_free(&check);
        return false;
    }
    app.workspace.map->scene = candidate;
    editor_commit();
    pe_assets_free(&app.workspace.map->assets);
    app.workspace.map->assets = check;
    app.workspace.map->view.tile = 1;
    app.workspace.map->view.palette_scroll = 0;
    editor_message("Tileset selected (Ctrl+Z to undo)");
    return true;
}

bool editor_import_png(const char *source) {
    if (app.workspace.map->view.playing || !editor_suffix(source, ".png")) {
        editor_message("Import requires a PNG file; stop play mode first");
        return false;
    }

    SDL_PathInfo info;
    if (!SDL_GetPathInfo(source, &info) || info.type != SDL_PATHTYPE_FILE ||
        info.size > 64 * 1024 * 1024) {
        editor_message("Cannot import file (max 64 MiB)");
        return false;
    }

    char folder[1024];
    snprintf(folder, sizeof folder, "%s/assets", app.project.root);
    if (!SDL_CreateDirectory(folder)) {
        editor_message(SDL_GetError());
        return false;
    }

    char relative[PE_PATH], destination[1024];
    int counter = 0;
    do {
        snprintf(relative, sizeof relative, "assets/import_%llu_%d.png",
                 (unsigned long long)SDL_GetTicksNS(), counter++);
        snprintf(destination, sizeof destination, "%s/%s", app.project.root, relative);
    } while (SDL_GetPathInfo(destination, &info) && info.type != SDL_PATHTYPE_NONE);
    if (!SDL_CopyFile(source, destination)) {
        editor_message(SDL_GetError());
        return false;
    }
    if (!editor_set_tileset(relative, app.workspace.map->scene.tileset_tile_size)) {
        SDL_RemovePath(destination);
        return false;
    }
    editor_tree_refresh();
    editor_message("PNG imported; undo restores previous tileset, file stays available");
    return true;
}
