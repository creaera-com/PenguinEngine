#ifndef PENGUIN_ENGINE_H
#define PENGUIN_ENGINE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define PE_ENTITIES 128
#define PE_LAYERS 4
#define PE_MAP_MAX 64
#define PE_PATH 256
#define PE_HISTORY 64
#define PE_TRANSFORM 1u
#define PE_SPRITE 2u
#define PE_BODY 4u
#define PE_PLAYER 8u
typedef struct {
    uint32_t index, generation;
} PeEntity;
typedef struct {
    float x, y, w, h, vx, vy;
    uint32_t color;
    uint32_t mask;
    uint32_t stable_id;
    uint32_t generation;
    bool alive;
} PeObject;
typedef struct {
    char name[32];
    bool visible;
    uint16_t tiles[PE_MAP_MAX * PE_MAP_MAX];
} PeLayer;
typedef struct {
    int width, height, tile_size, layer_count;
    char tileset[PE_PATH];
    int tileset_tile_size;
    PeLayer layers[PE_LAYERS];
    /* Collision is separate from visual tile layers. */
    uint8_t solid[PE_MAP_MAX * PE_MAP_MAX];
    PeObject objects[PE_ENTITIES];
    uint32_t next_id;
} PeScene;
typedef struct {
    float x, y, zoom;
} PeCamera;
typedef struct {
    PeScene *before, *after;
    uint64_t before_revision, after_revision;
} PeCommand;
typedef struct {
    PeCommand commands[PE_HISTORY];
    int count, cursor;
    PeScene *pending;
    uint64_t revision, next_revision, saved_revision;
} PeHistory;
void pe_scene_init(PeScene *scene, int width, int height, int tile_size);
PeEntity pe_entity_create(PeScene *scene, uint32_t mask);
bool pe_entity_valid(const PeScene *scene, PeEntity entity);
bool pe_entity_destroy(PeScene *scene, PeEntity entity);
PeObject *pe_entity_get(PeScene *scene, PeEntity entity);
PeEntity pe_entity_find(const PeScene *scene, uint32_t stable_id);
bool pe_tile_set(PeScene *scene, int layer, int x, int y, uint16_t tile);
uint16_t pe_tile_get(const PeScene *scene, int layer, int x, int y);
bool pe_fill(PeScene *scene, int layer, int x, int y, uint16_t tile);
void pe_rectangle(PeScene *scene, int layer, int x0, int y0, int x1, int y1, uint16_t tile);
void pe_history_init(PeHistory *history);
void pe_history_free(PeHistory *history);
bool pe_history_begin(PeHistory *history, const PeScene *scene);
bool pe_history_commit(PeHistory *history, PeScene *scene);
void pe_history_cancel(PeHistory *history, PeScene *scene);
bool pe_undo(PeHistory *history, PeScene *scene);
bool pe_redo(PeHistory *history, PeScene *scene);
void pe_history_saved(PeHistory *history);
bool pe_history_dirty(const PeHistory *history);
void pe_camera_zoom(PeCamera *camera, float screen_x, float screen_y, float factor);
void pe_screen_to_world(const PeCamera *camera, float sx, float sy, float *wx, float *wy);
/* Rectangle against collision grid, including world boundaries. */
bool pe_collides(const PeScene *scene, float x, float y, float w, float h);
void pe_move(PeScene *scene, PeObject *object, float dx, float dy);
void pe_update(PeScene *scene, float input_x, float input_y, float dt);
/* On failure load leaves destination unchanged. Errors are caller-owned buffers. */
bool pe_scene_save(const PeScene *scene, const char *path, char *error, size_t size);
bool pe_scene_load(PeScene *scene, const char *path, char *error, size_t size);
bool pe_project_load(const char *root, char *scene_path, size_t size, char *error,
                     size_t error_size);
#endif
