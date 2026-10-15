#include "cJSON.h"
#include "penguin/engine.h"
#include "unity.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static PeScene scene, original;
static PeHistory history;
static char error[256];
#define FILE_NAME "hierarchy-test.json"

void setUp(void) {
    pe_scene_init(&scene, 16, 16, 16);
    strcpy(scene.tileset, "assets/tiles.png");
    pe_history_init(&history);
}

void tearDown(void) {
    pe_history_free(&history);
    remove(FILE_NAME);
    remove(FILE_NAME ".tmp");
}

static PeEntity entity(float x, float y) {
    PeEntity e = pe_entity_create(&scene, PE_TRANSFORM | PE_SPRITE);
    TEST_ASSERT_TRUE(pe_entity_valid(&scene, e));
    scene.objects[e.index].x = x;
    scene.objects[e.index].y = y;
    return e;
}

static void position(PeEntity e, float expected_x, float expected_y) {
    float x, y;
    TEST_ASSERT_TRUE(pe_entity_world_position(&scene, e, &x, &y));
    TEST_ASSERT_FLOAT_WITHIN(.001f, expected_x, x);
    TEST_ASSERT_FLOAT_WITHIN(.001f, expected_y, y);
}

static void test_local_offsets_and_world_edit(void) {
    PeEntity root = entity(20, 30), child = entity(4, 5), leaf = entity(2, 3);
    TEST_ASSERT_TRUE(pe_entity_set_parent(&scene, child, root, false));
    TEST_ASSERT_TRUE(pe_entity_set_parent(&scene, leaf, child, false));
    position(leaf, 26, 38);
    TEST_ASSERT_TRUE(pe_entity_set_world_position(&scene, root, 40, 50));
    position(child, 44, 55);
    position(leaf, 46, 58);
    TEST_ASSERT_TRUE(pe_entity_set_world_position(&scene, child, 60, 70));
    TEST_ASSERT_EQUAL_FLOAT(20, scene.objects[child.index].x);
    position(root, 40, 50);
    position(leaf, 62, 73);
}

static void test_reparent_and_detach_modes(void) {
    PeEntity a = entity(10, 20), b = entity(30, 40), child = entity(5, 6);
    TEST_ASSERT_TRUE(pe_entity_set_parent(&scene, child, a, false));
    position(child, 15, 26);
    TEST_ASSERT_TRUE(pe_entity_set_parent(&scene, child, b, true));
    position(child, 15, 26);
    TEST_ASSERT_EQUAL_FLOAT(-15, scene.objects[child.index].x);
    TEST_ASSERT_TRUE(pe_entity_set_parent(&scene, child, (PeEntity){UINT32_MAX, 0}, true));
    position(child, 15, 26);
    TEST_ASSERT_EQUAL_UINT32(0, scene.objects[child.index].parent_id);
    TEST_ASSERT_TRUE(pe_entity_set_parent(&scene, child, a, false));
    position(child, 25, 46);
}

static void test_invalid_parent_changes_are_atomic(void) {
    PeEntity root = entity(10, 10), child = entity(2, 2), stale = entity(0, 0);
    TEST_ASSERT_TRUE(pe_entity_set_parent(&scene, child, root, false));
    TEST_ASSERT_TRUE(pe_entity_destroy(&scene, stale));
    original = scene;
    TEST_ASSERT_FALSE(pe_entity_set_parent(&scene, root, child, false));
    TEST_ASSERT_FALSE(pe_entity_set_parent(&scene, root, root, true));
    TEST_ASSERT_FALSE(pe_entity_set_parent(&scene, child, stale, true));
    TEST_ASSERT_FALSE(pe_entity_set_parent(&scene, stale, root, true));
    TEST_ASSERT_FALSE(pe_entity_set_parent(&scene, child, (PeEntity){UINT32_MAX, 1}, true));
    TEST_ASSERT_FALSE(pe_entity_set_world_position(&scene, child, NAN, 10));
    TEST_ASSERT_EQUAL_MEMORY(&original, &scene, sizeof scene);
    scene.objects[root.index].mask = 0;
    TEST_ASSERT_FALSE(pe_entity_set_parent(&scene, child, root, false));
    TEST_ASSERT_FALSE(pe_scene_hierarchy_valid(&scene));
}

static void test_full_capacity_depth_is_bounded(void) {
    PeEntity root = entity(1, 1), previous = root;
    for (int i = 1; i < PE_ENTITIES; i++) {
        PeEntity next = entity(1, 1);
        TEST_ASSERT_TRUE(pe_entity_set_parent(&scene, next, previous, false));
        previous = next;
    }
    position(previous, PE_ENTITIES, PE_ENTITIES);
    TEST_ASSERT_TRUE(pe_scene_hierarchy_valid(&scene));
    TEST_ASSERT_FALSE(pe_entity_set_parent(&scene, root, previous, false));
    /* Even corrupted graphs must terminate instead of looping forever. */
    scene.objects[root.index].parent_id = scene.objects[previous.index].stable_id;
    float x = 99, y = 99;
    TEST_ASSERT_FALSE(pe_entity_world_position(&scene, root, &x, &y));
    TEST_ASSERT_EQUAL_FLOAT(99, x);
    TEST_ASSERT_FALSE(pe_scene_hierarchy_valid(&scene));
}

static void test_delete_subtree_invalidates_all_handles(void) {
    PeEntity root = entity(10, 10), child = entity(2, 2), leaf = entity(1, 1),
             other = entity(50, 50);
    TEST_ASSERT_TRUE(pe_entity_set_parent(&scene, child, root, false));
    TEST_ASSERT_TRUE(pe_entity_set_parent(&scene, leaf, child, false));
    TEST_ASSERT_TRUE(pe_entity_destroy(&scene, root));
    TEST_ASSERT_FALSE(pe_entity_valid(&scene, child));
    TEST_ASSERT_FALSE(pe_entity_valid(&scene, leaf));
    TEST_ASSERT_TRUE(pe_entity_valid(&scene, other));
    PeEntity replacement = entity(0, 0);
    TEST_ASSERT_FALSE(pe_entity_valid(&scene, root));
    TEST_ASSERT_NOT_EQUAL(root.generation, replacement.generation);
    TEST_ASSERT_EQUAL_UINT32(0, scene.objects[replacement.index].parent_id);
}

static void test_hierarchy_undo_redo(void) {
    PeEntity root = entity(20, 30), child = entity(4, 5);
    TEST_ASSERT_TRUE(pe_history_begin(&history, &scene));
    TEST_ASSERT_TRUE(pe_entity_set_parent(&scene, child, root, false));
    TEST_ASSERT_TRUE(pe_history_commit(&history, &scene));
    TEST_ASSERT_TRUE(pe_history_begin(&history, &scene));
    TEST_ASSERT_TRUE(pe_entity_set_world_position(&scene, root, 40, 50));
    TEST_ASSERT_TRUE(pe_history_commit(&history, &scene));
    TEST_ASSERT_TRUE(pe_history_begin(&history, &scene));
    TEST_ASSERT_TRUE(pe_entity_destroy(&scene, root));
    TEST_ASSERT_TRUE(pe_history_commit(&history, &scene));
    TEST_ASSERT_TRUE(pe_undo(&history, &scene));
    position(child, 44, 55);
    TEST_ASSERT_TRUE(pe_undo(&history, &scene));
    position(child, 24, 35);
    TEST_ASSERT_TRUE(pe_undo(&history, &scene));
    position(child, 4, 5);
    TEST_ASSERT_TRUE(pe_redo(&history, &scene));
    TEST_ASSERT_TRUE(pe_redo(&history, &scene));
    position(child, 44, 55);
    TEST_ASSERT_TRUE(pe_redo(&history, &scene));
    TEST_ASSERT_FALSE(pe_entity_valid(&scene, child));
}

static void test_child_collision_uses_world_coordinates(void) {
    PeEntity root = entity(32, 16), child = entity(0, 0);
    TEST_ASSERT_TRUE(pe_entity_set_parent(&scene, child, root, false));
    for (int y = 0; y < scene.height; y++) {
        scene.solid[y * PE_MAP_MAX + 3] = 1;
    }
    pe_move(&scene, &scene.objects[child.index], 100, 0);
    float x, y;
    TEST_ASSERT_TRUE(pe_entity_world_position(&scene, child, &x, &y));
    TEST_ASSERT_TRUE(x + scene.objects[child.index].w <= 48);
    TEST_ASSERT_TRUE(x >= 32);
    position(root, 32, 16);
}

static void test_runtime_updates_parents_before_children(void) {
    PeEntity child = entity(16, 0), parent = entity(20, 16);
    TEST_ASSERT_TRUE(pe_entity_set_parent(&scene, child, parent, false));
    scene.objects[parent.index].mask |= PE_BODY;
    scene.objects[child.index].mask |= PE_BODY;
    scene.objects[parent.index].vx = -10;
    scene.objects[child.index].vx = 10;
    for (int y = 0; y < scene.height; y++) {
        scene.solid[y * PE_MAP_MAX + 3] = 1;
    }
    pe_update(&scene, 0, 0, .1f);
    position(parent, 19, 16);
    position(child, 36, 16);
    TEST_ASSERT_EQUAL_FLOAT(17, scene.objects[child.index].x);
}

static cJSON *read_json(void) {
    FILE *file = fopen(FILE_NAME, "rb");
    TEST_ASSERT_NOT_NULL(file);
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    rewind(file);
    char *text = malloc((size_t)length + 1);
    TEST_ASSERT_NOT_NULL(text);
    TEST_ASSERT_EQUAL_UINT(length, fread(text, 1, (size_t)length, file));
    fclose(file);
    text[length] = 0;
    cJSON *json = cJSON_Parse(text);
    free(text);
    TEST_ASSERT_NOT_NULL(json);
    return json;
}

static void write_json(cJSON *json) {
    char *text = cJSON_PrintUnformatted(json);
    TEST_ASSERT_NOT_NULL(text);
    FILE *file = fopen(FILE_NAME, "wb");
    TEST_ASSERT_NOT_NULL(file);
    TEST_ASSERT_EQUAL_UINT(strlen(text), fwrite(text, 1, strlen(text), file));
    fclose(file);
    free(text);
    cJSON_Delete(json);
}

static void test_roundtrip_and_legacy_scene(void) {
    PeEntity child = entity(4, 5), parent = entity(20, 30);
    TEST_ASSERT_TRUE(pe_entity_set_parent(&scene, child, parent, false));
    TEST_ASSERT_TRUE_MESSAGE(pe_scene_save(&scene, FILE_NAME, error, sizeof error), error);
    original = scene;
    pe_scene_init(&scene, 1, 1, 1);
    TEST_ASSERT_TRUE_MESSAGE(pe_scene_load(&scene, FILE_NAME, error, sizeof error), error);
    position(pe_entity_find(&scene, original.objects[child.index].stable_id), 24, 35);
    cJSON *json = read_json();
    cJSON_ReplaceItemInObjectCaseSensitive(json, "version", cJSON_CreateNumber(1));
    cJSON *objects = cJSON_GetObjectItemCaseSensitive(json, "entities");
    for (cJSON *o = objects->child; o; o = o->next) {
        cJSON_DeleteItemFromObjectCaseSensitive(o, "parent");
    }
    write_json(json);
    TEST_ASSERT_TRUE(pe_scene_load(&scene, FILE_NAME, error, sizeof error));
    position(pe_entity_find(&scene, original.objects[child.index].stable_id), 4, 5);
}

static void test_malformed_hierarchy_load_is_transactional(void) {
    PeEntity parent = entity(20, 30), child = entity(4, 5);
    TEST_ASSERT_TRUE(pe_entity_set_parent(&scene, child, parent, false));
    original = scene;
    for (int variant = 0; variant < 9; variant++) {
        TEST_ASSERT_TRUE(pe_scene_save(&scene, FILE_NAME, error, sizeof error));
        cJSON *json = read_json();
        cJSON *objects = cJSON_GetObjectItemCaseSensitive(json, "entities");
        cJSON *a = cJSON_GetArrayItem(objects, 0), *b = cJSON_GetArrayItem(objects, 1);
        switch (variant) {
        case 0:
            cJSON_ReplaceItemInObjectCaseSensitive(b, "parent", cJSON_CreateNumber(999));
            break;
        case 1:
            cJSON_ReplaceItemInObjectCaseSensitive(a, "parent", cJSON_CreateNumber(2));
            break;
        case 2:
            cJSON_ReplaceItemInObjectCaseSensitive(b, "parent", cJSON_CreateNumber(2));
            break;
        case 3:
            cJSON_DeleteItemFromObjectCaseSensitive(b, "parent");
            break;
        case 4:
            cJSON_ReplaceItemInObjectCaseSensitive(b, "parent", cJSON_CreateNumber(.5));
            break;
        case 5:
            cJSON_ReplaceItemInObjectCaseSensitive(b, "mask", cJSON_CreateNumber(0));
            break;
        case 6:
            cJSON_ReplaceItemInObjectCaseSensitive(a, "mask", cJSON_CreateNumber(0));
            break;
        case 7:
            cJSON_ReplaceItemInObjectCaseSensitive(json, "version", cJSON_CreateNumber(1));
            break;
        case 8:
            cJSON_ReplaceItemInObjectCaseSensitive(b, "parent", cJSON_CreateNumber(-1));
            break;
        }
        write_json(json);
        TEST_ASSERT_FALSE(pe_scene_load(&scene, FILE_NAME, error, sizeof error));
        TEST_ASSERT_EQUAL_MEMORY(&original, &scene, sizeof scene);
    }
    scene.objects[parent.index].parent_id = scene.objects[child.index].stable_id;
    TEST_ASSERT_FALSE(pe_scene_save(&scene, FILE_NAME, error, sizeof error));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_local_offsets_and_world_edit);
    RUN_TEST(test_reparent_and_detach_modes);
    RUN_TEST(test_invalid_parent_changes_are_atomic);
    RUN_TEST(test_full_capacity_depth_is_bounded);
    RUN_TEST(test_delete_subtree_invalidates_all_handles);
    RUN_TEST(test_hierarchy_undo_redo);
    RUN_TEST(test_child_collision_uses_world_coordinates);
    RUN_TEST(test_runtime_updates_parents_before_children);
    RUN_TEST(test_roundtrip_and_legacy_scene);
    RUN_TEST(test_malformed_hierarchy_load_is_transactional);
    return UNITY_END();
}
