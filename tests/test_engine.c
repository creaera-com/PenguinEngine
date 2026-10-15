#include "cJSON.h"
#include "penguin/engine.h"
#include "unity.h"
#include <math.h>
#ifdef _WIN32
#include <direct.h>
#define make_dir(path) _mkdir(path)
#define remove_dir(path) _rmdir(path)
#else
#include <sys/stat.h>
#include <unistd.h>
#define make_dir(path) mkdir(path, 0700)
#define remove_dir(path) rmdir(path)
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static PeScene scene, original;
static PeHistory history;
static char error[256];

void setUp(void) {
    pe_scene_init(&scene, 8, 6, 16);
    strcpy(scene.tileset, "assets/tiles.png");
    pe_history_init(&history);
    error[0] = 0;
}

void tearDown(void) {
    pe_history_free(&history);
    remove("test_scene.json");
    remove("test_scene.json.tmp");
    remove("invalid_scene.json");
}

static void test_stale_entity(void) {
    PeEntity old = pe_entity_create(&scene, PE_TRANSFORM);
    TEST_ASSERT_TRUE(pe_entity_destroy(&scene, old));
    PeEntity replacement = pe_entity_create(&scene, PE_SPRITE);
    TEST_ASSERT_EQUAL_UINT32(old.index, replacement.index);
    TEST_ASSERT_FALSE(pe_entity_valid(&scene, old));
    TEST_ASSERT_NULL(pe_entity_get(&scene, old));
    TEST_ASSERT_TRUE(pe_entity_valid(&scene, replacement));
}

static void test_capacity(void) {
    for (int i = 0; i < PE_ENTITIES; i++) {
        TEST_ASSERT_TRUE(pe_entity_valid(&scene, pe_entity_create(&scene, PE_TRANSFORM)));
    }
    TEST_ASSERT_FALSE(pe_entity_valid(&scene, pe_entity_create(&scene, PE_TRANSFORM)));
}

static void test_stable_lookup(void) {
    PeEntity e = pe_entity_create(&scene, PE_PLAYER);
    TEST_ASSERT_EQUAL_UINT32(e.index,
                             pe_entity_find(&scene, pe_entity_get(&scene, e)->stable_id).index);
    TEST_ASSERT_FALSE(pe_entity_valid(&scene, pe_entity_find(&scene, 900)));
}

static void test_tile_bounds(void) {
    TEST_ASSERT_FALSE(pe_tile_set(&scene, 0, -1, 0, 1));
    TEST_ASSERT_FALSE(pe_tile_set(&scene, 0, 8, 0, 1));
    TEST_ASSERT_FALSE(pe_tile_set(&scene, 1, 0, 0, 1));
    TEST_ASSERT_TRUE(pe_tile_set(&scene, 0, 7, 5, 9));
    TEST_ASSERT_EQUAL_INT(9, pe_tile_get(&scene, 0, 7, 5));
}

static void test_flood_barrier(void) {
    for (int y = 0; y < 6; y++) {
        pe_tile_set(&scene, 0, 3, y, 2);
    }
    TEST_ASSERT_TRUE(pe_fill(&scene, 0, 0, 0, 5));
    TEST_ASSERT_EQUAL_INT(5, pe_tile_get(&scene, 0, 2, 5));
    TEST_ASSERT_EQUAL_INT(2, pe_tile_get(&scene, 0, 3, 2));
    TEST_ASSERT_EQUAL_INT(0, pe_tile_get(&scene, 0, 4, 2));
}

static void test_flood_full_map(void) {
    pe_scene_init(&scene, 64, 64, 16);
    TEST_ASSERT_TRUE(pe_fill(&scene, 0, 0, 0, 7));
    TEST_ASSERT_EQUAL_INT(7, pe_tile_get(&scene, 0, 63, 63));
    TEST_ASSERT_TRUE(pe_fill(&scene, 0, 2, 3, 7));
}

static void test_rectangle_clips(void) {
    pe_rectangle(&scene, 0, 99, 99, -10, -10, 4);
    TEST_ASSERT_EQUAL_INT(4, pe_tile_get(&scene, 0, 0, 0));
    TEST_ASSERT_EQUAL_INT(4, pe_tile_get(&scene, 0, 7, 5));
}

static void change(int value) {
    TEST_ASSERT_TRUE(pe_history_begin(&history, &scene));
    pe_tile_set(&scene, 0, 1, 1, (uint16_t)value);
    TEST_ASSERT_TRUE(pe_history_commit(&history, &scene));
}

static void test_stroke_undo(void) {
    TEST_ASSERT_TRUE(pe_history_begin(&history, &scene));
    pe_tile_set(&scene, 0, 1, 1, 3);
    pe_tile_set(&scene, 0, 2, 1, 3);
    pe_tile_set(&scene, 0, 1, 1, 4);
    TEST_ASSERT_TRUE(pe_history_commit(&history, &scene));
    TEST_ASSERT_EQUAL_INT(1, history.count);
    TEST_ASSERT_TRUE(pe_undo(&history, &scene));
    TEST_ASSERT_EQUAL_INT(0, pe_tile_get(&scene, 0, 1, 1));
    TEST_ASSERT_EQUAL_INT(0, pe_tile_get(&scene, 0, 2, 1));
    TEST_ASSERT_TRUE(pe_redo(&history, &scene));
    TEST_ASSERT_EQUAL_INT(4, pe_tile_get(&scene, 0, 1, 1));
}

static void test_redo_branch(void) {
    change(1);
    change(2);
    TEST_ASSERT_TRUE(pe_undo(&history, &scene));
    change(3);
    TEST_ASSERT_FALSE(pe_redo(&history, &scene));
    TEST_ASSERT_TRUE(pe_undo(&history, &scene));
    TEST_ASSERT_EQUAL_INT(1, pe_tile_get(&scene, 0, 1, 1));
}

static void test_saved_marker(void) {
    change(1);
    pe_history_saved(&history);
    TEST_ASSERT_FALSE(pe_history_dirty(&history));
    change(2);
    TEST_ASSERT_TRUE(pe_history_dirty(&history));
    pe_undo(&history, &scene);
    TEST_ASSERT_FALSE(pe_history_dirty(&history));
    pe_undo(&history, &scene);
    change(9);
    TEST_ASSERT_TRUE(pe_history_dirty(&history));
}

static void test_cancel_and_noop(void) {
    pe_history_begin(&history, &scene);
    TEST_ASSERT_FALSE(pe_history_begin(&history, &scene));
    pe_tile_set(&scene, 0, 1, 1, 3);
    TEST_ASSERT_FALSE(pe_undo(&history, &scene));
    pe_history_cancel(&history, &scene);
    TEST_ASSERT_EQUAL_INT(0, pe_tile_get(&scene, 0, 1, 1));
    pe_history_begin(&history, &scene);
    pe_history_commit(&history, &scene);
    TEST_ASSERT_EQUAL_INT(0, history.count);
}

static void test_history_limit(void) {
    for (int i = 1; i <= 100; i++) {
        change(i);
    }
    TEST_ASSERT_EQUAL_INT(PE_HISTORY, history.count);
    for (int i = 0; i < PE_HISTORY; i++) {
        TEST_ASSERT_TRUE(pe_undo(&history, &scene));
    }
    TEST_ASSERT_EQUAL_INT(36, pe_tile_get(&scene, 0, 1, 1));
    TEST_ASSERT_FALSE(pe_undo(&history, &scene));
}

static void test_entity_delete_undo(void) {
    PeEntity e = pe_entity_create(&scene, PE_TRANSFORM | PE_PLAYER);
    uint32_t id = pe_entity_get(&scene, e)->stable_id;
    pe_history_begin(&history, &scene);
    pe_entity_destroy(&scene, e);
    pe_history_commit(&history, &scene);
    TEST_ASSERT_FALSE(pe_entity_valid(&scene, pe_entity_find(&scene, id)));
    pe_undo(&history, &scene);
    TEST_ASSERT_TRUE(pe_entity_valid(&scene, pe_entity_find(&scene, id)));
}

static void test_zoom_anchor(void) {
    PeCamera c = {20, 40, 2};
    float x, y, x2, y2;
    pe_screen_to_world(&c, 124, 93, &x, &y);
    pe_camera_zoom(&c, 124, 93, 1.8f);
    pe_screen_to_world(&c, 124, 93, &x2, &y2);
    TEST_ASSERT_FLOAT_WITHIN(.0001f, x, x2);
    TEST_ASSERT_FLOAT_WITHIN(.0001f, y, y2);
    pe_camera_zoom(&c, 0, 0, 100);
    TEST_ASSERT_EQUAL_FLOAT(8, c.zoom);
}

static void test_collision_edges(void) {
    scene.solid[2 * PE_MAP_MAX + 2] = 1;
    TEST_ASSERT_FALSE(pe_collides(&scene, 16, 32, 16, 16));
    TEST_ASSERT_TRUE(pe_collides(&scene, 17, 32, 16, 16));
    TEST_ASSERT_TRUE(pe_collides(&scene, -1, 0, 2, 2));
    TEST_ASSERT_FALSE(pe_collides(&scene, 0, 0, 16, 16));
}

static void test_no_tunneling(void) {
    for (int y = 0; y < 6; y++) {
        scene.solid[y * PE_MAP_MAX + 3] = 1;
    }

    PeObject o = {0};
    o.x = 16;
    o.y = 16;
    o.w = 12;
    o.h = 12;
    pe_move(&scene, &o, 100, 16);
    TEST_ASSERT_TRUE(o.x + o.w <= 48);
    TEST_ASSERT_FLOAT_WITHIN(.01f, 32, o.y);
}

static void test_normalized_player(void) {
    PeObject *o = pe_entity_get(&scene, pe_entity_create(&scene, 15));
    o->x = 32;
    o->y = 32;
    pe_update(&scene, 1, 1, 1.f / 60);
    TEST_ASSERT_FLOAT_WITHIN(.001f, 90, sqrtf(o->vx * o->vx + o->vy * o->vy));
}

static void test_scene_roundtrip(void) {
    PeObject *o = pe_entity_get(&scene, pe_entity_create(&scene, 15));
    o->x = 21.5f;
    o->stable_id = 42;
    pe_tile_set(&scene, 0, 3, 4, 17);
    scene.solid[3] = 1;
    scene.layer_count = 2;
    strcpy(scene.layers[1].name, "Decor");
    scene.layers[1].visible = true;
    pe_tile_set(&scene, 1, 2, 2, 6);
    TEST_ASSERT_TRUE_MESSAGE(pe_scene_save(&scene, "test_scene.json", error, sizeof error), error);
    PeScene loaded;
    TEST_ASSERT_TRUE_MESSAGE(pe_scene_load(&loaded, "test_scene.json", error, sizeof error), error);
    TEST_ASSERT_EQUAL_INT(17, pe_tile_get(&loaded, 0, 3, 4));
    TEST_ASSERT_EQUAL_INT(6, pe_tile_get(&loaded, 1, 2, 2));
    TEST_ASSERT_EQUAL_INT(1, loaded.solid[3]);
    TEST_ASSERT_EQUAL_STRING(scene.tileset, loaded.tileset);
    TEST_ASSERT_EQUAL_FLOAT(21.5f, pe_entity_get(&loaded, pe_entity_find(&loaded, 42))->x);
    TEST_ASSERT_TRUE(loaded.next_id > 42);
}

static void test_invalid_load_transaction(void) {
    original = scene;
    FILE *f = fopen("invalid_scene.json", "wb");
    TEST_ASSERT_NOT_NULL(f);
    fputs("{\"version\":99}", f);
    fclose(f);
    TEST_ASSERT_FALSE(pe_scene_load(&scene, "invalid_scene.json", error, sizeof error));
    TEST_ASSERT_EQUAL_MEMORY(&original, &scene, sizeof scene);
}

static void test_trailing_json_rejected(void) {
    FILE *f = fopen("invalid_scene.json", "wb");
    TEST_ASSERT_NOT_NULL(f);
    fputs("{} garbage", f);
    fclose(f);
    TEST_ASSERT_FALSE(pe_scene_load(&scene, "invalid_scene.json", error, sizeof error));
}

static void test_missing_file(void) {
    original = scene;
    TEST_ASSERT_FALSE(pe_scene_load(&scene, "file_that_does_not_exist", error, sizeof error));
    TEST_ASSERT_EQUAL_MEMORY(&original, &scene, sizeof scene);
}

static cJSON *fixture(void) {
    TEST_ASSERT_TRUE(pe_scene_save(&scene, "test_scene.json", error, sizeof error));
    FILE *f = fopen("test_scene.json", "rb");
    TEST_ASSERT_NOT_NULL(f);
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    char *data = malloc((size_t)size + 1);
    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_EQUAL_INT(size, fread(data, 1, (size_t)size, f));
    fclose(f);
    data[size] = 0;
    cJSON *j = cJSON_Parse(data);
    free(data);
    TEST_ASSERT_NOT_NULL(j);
    return j;
}

static void reject(cJSON *j) {
    original = scene;
    char *data = cJSON_Print(j);
    TEST_ASSERT_NOT_NULL(data);
    FILE *f = fopen("invalid_scene.json", "wb");
    TEST_ASSERT_NOT_NULL(f);
    fputs(data, f);
    fclose(f);
    free(data);
    cJSON_Delete(j);
    TEST_ASSERT_FALSE(pe_scene_load(&scene, "invalid_scene.json", error, sizeof error));
    TEST_ASSERT_EQUAL_MEMORY(&original, &scene, sizeof scene);
}

static void test_asset_traversal_rejected(void) {
    cJSON *j = fixture();
    cJSON_ReplaceItemInObject(j, "tileset", cJSON_CreateString("../secret.png"));
    reject(j);
}

static void test_absolute_asset_rejected(void) {
    cJSON *j = fixture();
    cJSON_ReplaceItemInObject(j, "tileset", cJSON_CreateString("C:/images/a.png"));
    reject(j);
}

static void test_fractional_map_size_rejected(void) {
    cJSON *j = fixture();
    cJSON_ReplaceItemInObject(j, "width", cJSON_CreateNumber(4.5));
    reject(j);
}

static void test_missing_tiles_rejected(void) {
    cJSON *j = fixture();
    cJSON *tiles =
        cJSON_GetObjectItem(cJSON_GetArrayItem(cJSON_GetObjectItem(j, "layers"), 0), "tiles");
    cJSON_DeleteItemFromArray(tiles, 0);
    reject(j);
}

static void test_duplicate_entity_ids_rejected(void) {
    pe_entity_create(&scene, 3);
    pe_entity_create(&scene, 3);
    cJSON *j = fixture();
    cJSON *entities = cJSON_GetObjectItem(j, "entities");
    cJSON_ReplaceItemInObject(cJSON_GetArrayItem(entities, 1), "id", cJSON_CreateNumber(1));
    reject(j);
}

static void test_negative_body_size_rejected(void) {
    pe_entity_create(&scene, 3);
    cJSON *j = fixture();
    cJSON_ReplaceItemInObject(cJSON_GetArrayItem(cJSON_GetObjectItem(j, "entities"), 0), "w",
                              cJSON_CreateNumber(-4));
    reject(j);
}

static void test_collision_values_rejected(void) {
    cJSON *j = fixture();
    cJSON_ReplaceItemInArray(cJSON_GetObjectItem(j, "collision"), 0, cJSON_CreateNumber(2));
    reject(j);
}

static void test_layer_and_collision_undo(void) {
    pe_history_begin(&history, &scene);
    scene.layer_count = 2;
    strcpy(scene.layers[1].name, "Objects");
    scene.solid[0] = 1;
    strcpy(scene.tileset, "assets/new.png");
    pe_history_commit(&history, &scene);
    pe_undo(&history, &scene);
    TEST_ASSERT_EQUAL_INT(1, scene.layer_count);
    TEST_ASSERT_EQUAL_INT(0, scene.solid[0]);
    TEST_ASSERT_EQUAL_STRING("assets/tiles.png", scene.tileset);
    pe_redo(&history, &scene);
    TEST_ASSERT_EQUAL_INT(2, scene.layer_count);
    TEST_ASSERT_EQUAL_INT(1, scene.solid[0]);
}

static void test_nonfinite_collision_and_movement(void) {
    TEST_ASSERT_TRUE(pe_collides(&scene, 0, 0, NAN, 12));
    PeObject o = {0};
    o.w = o.h = 12;
    pe_move(&scene, &o, INFINITY, 2);
    TEST_ASSERT_EQUAL_FLOAT(0, o.x);
    pe_move(&scene, &o, 1e30f, 0);
    TEST_ASSERT_EQUAL_FLOAT(0, o.x);
}

static void test_runtime_copy_isolation(void) {
    PeScene runtime = scene;
    pe_tile_set(&runtime, 0, 1, 1, 9);
    PeObject *o = pe_entity_get(&runtime, pe_entity_create(&runtime, 15));
    o->x = 32;
    o->y = 32;
    pe_update(&runtime, 1, 0, 1.f / 60);
    TEST_ASSERT_EQUAL_INT(0, pe_tile_get(&scene, 0, 1, 1));
    TEST_ASSERT_FALSE(scene.objects[0].alive);
}

static void test_save_failure_retains_file(void) {
    TEST_ASSERT_TRUE(pe_scene_save(&scene, "test_scene.json", error, sizeof error));
    pe_tile_set(&scene, 0, 1, 1, 5);
    TEST_ASSERT_FALSE(
        pe_scene_save(&scene, "missing-directory/test_scene.json", error, sizeof error));
    PeScene loaded;
    TEST_ASSERT_TRUE(pe_scene_load(&loaded, "test_scene.json", error, sizeof error));
    TEST_ASSERT_EQUAL_INT(0, pe_tile_get(&loaded, 0, 1, 1));
}

static void test_project_resolution(void) {
    make_dir("test-resolution");
    make_dir("test-resolution/example");
    make_dir("test-resolution/Debug");
    FILE *file = fopen("test-resolution/example/project.json", "wb");
    TEST_ASSERT_NOT_NULL(file);
    fputs("{}", file);
    fclose(file);
    char root[768];
    bool beside =
        pe_project_resolve(NULL, "test-resolution", root, sizeof root, error, sizeof error);
    bool beside_path = !strcmp(root, "test-resolution/example");
    bool parent =
        pe_project_resolve(NULL, "test-resolution/Debug", root, sizeof root, error, sizeof error);
    bool parent_path = !strcmp(root, "test-resolution/Debug/../example");
    bool explicit_ok =
        pe_project_resolve("test-resolution/example", NULL, root, sizeof root, error, sizeof error);
    bool explicit_path = !strcmp(root, "test-resolution/example");
    bool invalid = pe_project_resolve("test-resolution/missing", "test-resolution", root,
                                      sizeof root, error, sizeof error);
    char short_root[2];
    bool truncated = pe_project_resolve("test-resolution/example", NULL, short_root,
                                        sizeof short_root, error, sizeof error);
    remove("test-resolution/example/project.json");
    remove_dir("test-resolution/example");
    remove_dir("test-resolution/Debug");
    remove_dir("test-resolution");
    TEST_ASSERT_TRUE(beside && beside_path);
    TEST_ASSERT_TRUE(parent && parent_path);
    TEST_ASSERT_TRUE(explicit_ok && explicit_path);
    TEST_ASSERT_FALSE(invalid);
    TEST_ASSERT_FALSE(truncated);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_stale_entity);
    RUN_TEST(test_capacity);
    RUN_TEST(test_stable_lookup);
    RUN_TEST(test_tile_bounds);
    RUN_TEST(test_flood_barrier);
    RUN_TEST(test_flood_full_map);
    RUN_TEST(test_rectangle_clips);
    RUN_TEST(test_stroke_undo);
    RUN_TEST(test_redo_branch);
    RUN_TEST(test_saved_marker);
    RUN_TEST(test_cancel_and_noop);
    RUN_TEST(test_history_limit);
    RUN_TEST(test_entity_delete_undo);
    RUN_TEST(test_zoom_anchor);
    RUN_TEST(test_collision_edges);
    RUN_TEST(test_no_tunneling);
    RUN_TEST(test_normalized_player);
    RUN_TEST(test_scene_roundtrip);
    RUN_TEST(test_invalid_load_transaction);
    RUN_TEST(test_trailing_json_rejected);
    RUN_TEST(test_missing_file);
    RUN_TEST(test_asset_traversal_rejected);
    RUN_TEST(test_absolute_asset_rejected);
    RUN_TEST(test_fractional_map_size_rejected);
    RUN_TEST(test_missing_tiles_rejected);
    RUN_TEST(test_duplicate_entity_ids_rejected);
    RUN_TEST(test_negative_body_size_rejected);
    RUN_TEST(test_collision_values_rejected);
    RUN_TEST(test_layer_and_collision_undo);
    RUN_TEST(test_nonfinite_collision_and_movement);
    RUN_TEST(test_runtime_copy_isolation);
    RUN_TEST(test_save_failure_retains_file);
    RUN_TEST(test_project_resolution);
    return UNITY_END();
}
