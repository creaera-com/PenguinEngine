#include "cJSON.h"
#include "penguin/engine.h"
#include "unity.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static PeScene scene, original;
static PeHistory history;
static PeEntity entity;
static char error[256];
#define FILE_NAME "components-test.json"

void setUp(void) {
    pe_scene_init(&scene, 8, 8, 16);
    strcpy(scene.tileset, "assets/tiles.png");
    pe_history_init(&history);
    entity = pe_entity_create(&scene, 0);
}

void tearDown(void) {
    pe_history_free(&history);
    remove(FILE_NAME);
    remove(FILE_NAME ".tmp");
}

static void test_add_defaults_and_dependencies(void) {
    TEST_ASSERT_TRUE(pe_entity_add_component(&scene, entity, PE_SPRITE));
    PeObject *o = pe_entity_get(&scene, entity);
    TEST_ASSERT_EQUAL_UINT32(PE_TRANSFORM | PE_SPRITE, o->mask);
    TEST_ASSERT_EQUAL_FLOAT(12, o->w);
    o->x = 25;
    o->color = 0x12345678;
    TEST_ASSERT_TRUE(pe_entity_add_component(&scene, entity, PE_PLAYER));
    TEST_ASSERT_EQUAL_UINT32(15, o->mask);
    TEST_ASSERT_EQUAL_FLOAT(25, o->x);
    TEST_ASSERT_EQUAL_UINT32(0x12345678, o->color);
    TEST_ASSERT_EQUAL_FLOAT(0, o->vx);
}

static void test_invalid_add_and_stale_handles_are_atomic(void) {
    TEST_ASSERT_TRUE(pe_entity_add_component(&scene, entity, PE_BODY));
    original = scene;
    TEST_ASSERT_FALSE(pe_entity_add_component(&scene, entity, PE_BODY));
    TEST_ASSERT_FALSE(pe_entity_add_component(&scene, entity, 0));
    TEST_ASSERT_FALSE(pe_entity_add_component(&scene, entity, 16));
    TEST_ASSERT_FALSE(pe_entity_add_component(&scene, entity, PE_PLAYER | PE_SPRITE));
    TEST_ASSERT_EQUAL_MEMORY(&original, &scene, sizeof scene);
    TEST_ASSERT_TRUE(pe_entity_destroy(&scene, entity));
    original = scene;
    TEST_ASSERT_FALSE(pe_entity_add_component(&scene, entity, PE_PLAYER));
    TEST_ASSERT_FALSE(pe_entity_remove_component(&scene, entity, PE_BODY));
    TEST_ASSERT_EQUAL_MEMORY(&original, &scene, sizeof scene);
}

static void test_removal_protects_dependencies_and_hierarchy(void) {
    TEST_ASSERT_TRUE(pe_entity_add_component(&scene, entity, PE_PLAYER));
    original = scene;
    TEST_ASSERT_FALSE(pe_entity_remove_component(&scene, entity, PE_TRANSFORM));
    TEST_ASSERT_FALSE(pe_entity_remove_component(&scene, entity, PE_BODY));
    TEST_ASSERT_FALSE(pe_entity_remove_component(&scene, entity, PE_SPRITE));
    TEST_ASSERT_FALSE(pe_entity_remove_component(&scene, entity, 15));
    TEST_ASSERT_EQUAL_MEMORY(&original, &scene, sizeof scene);
    TEST_ASSERT_TRUE(pe_entity_remove_component(&scene, entity, PE_PLAYER));
    TEST_ASSERT_TRUE(pe_entity_remove_component(&scene, entity, PE_BODY));
    PeEntity child = pe_entity_create(&scene, PE_TRANSFORM);
    TEST_ASSERT_TRUE(pe_entity_set_parent(&scene, child, entity, false));
    TEST_ASSERT_FALSE(pe_entity_remove_component(&scene, entity, PE_TRANSFORM));
    TEST_ASSERT_FALSE(pe_entity_remove_component(&scene, child, PE_TRANSFORM));
    TEST_ASSERT_TRUE(pe_scene_hierarchy_valid(&scene));
    TEST_ASSERT_TRUE(pe_entity_set_parent(&scene, child, (PeEntity){UINT32_MAX, 0}, true));
    TEST_ASSERT_TRUE(pe_entity_remove_component(&scene, entity, PE_TRANSFORM));
    TEST_ASSERT_TRUE(pe_entity_valid(&scene, entity));
    TEST_ASSERT_EQUAL_UINT32(0, pe_entity_get(&scene, entity)->mask);
}

static void test_readded_components_get_defaults(void) {
    TEST_ASSERT_TRUE(pe_entity_add_component(&scene, entity, PE_BODY));
    PeObject *o = pe_entity_get(&scene, entity);
    o->vx = 30;
    TEST_ASSERT_TRUE(pe_entity_remove_component(&scene, entity, PE_BODY));
    TEST_ASSERT_TRUE(pe_entity_add_component(&scene, entity, PE_BODY));
    TEST_ASSERT_EQUAL_FLOAT(0, o->vx);
    TEST_ASSERT_TRUE(pe_entity_add_component(&scene, entity, PE_SPRITE));
    o->color = 0;
    TEST_ASSERT_TRUE(pe_entity_remove_component(&scene, entity, PE_SPRITE));
    TEST_ASSERT_TRUE(pe_entity_add_component(&scene, entity, PE_SPRITE));
    TEST_ASSERT_EQUAL_UINT32(0xffcc55ff, o->color);
}

static void test_component_operations_undo_together(void) {
    TEST_ASSERT_TRUE(pe_history_begin(&history, &scene));
    TEST_ASSERT_TRUE(pe_entity_add_component(&scene, entity, PE_PLAYER));
    TEST_ASSERT_TRUE(pe_history_commit(&history, &scene));
    TEST_ASSERT_EQUAL_INT(1, history.count);
    TEST_ASSERT_TRUE(pe_undo(&history, &scene));
    TEST_ASSERT_EQUAL_UINT32(0, pe_entity_get(&scene, entity)->mask);
    TEST_ASSERT_TRUE(pe_redo(&history, &scene));
    TEST_ASSERT_EQUAL_UINT32(PE_TRANSFORM | PE_BODY | PE_PLAYER,
                             pe_entity_get(&scene, entity)->mask);
    TEST_ASSERT_TRUE(pe_history_begin(&history, &scene));
    TEST_ASSERT_TRUE(pe_entity_remove_component(&scene, entity, PE_PLAYER));
    TEST_ASSERT_TRUE(pe_history_commit(&history, &scene));
    TEST_ASSERT_TRUE(pe_undo(&history, &scene));
    TEST_ASSERT_TRUE(pe_entity_get(&scene, entity)->mask & PE_PLAYER);
}

static void test_settings_roundtrip_and_optional_velocity(void) {
    TEST_ASSERT_TRUE(pe_entity_add_component(&scene, entity, PE_SPRITE));
    TEST_ASSERT_TRUE(pe_entity_add_component(&scene, entity, PE_BODY));
    PeObject *o = pe_entity_get(&scene, entity);
    o->vx = 12.5f;
    o->vy = -7.25f;
    o->color = 0x12345678;
    uint32_t id = o->stable_id;
    TEST_ASSERT_TRUE_MESSAGE(pe_scene_save(&scene, FILE_NAME, error, sizeof error), error);
    TEST_ASSERT_TRUE(pe_scene_load(&scene, FILE_NAME, error, sizeof error));
    o = pe_entity_get(&scene, pe_entity_find(&scene, id));
    TEST_ASSERT_EQUAL_FLOAT(12.5f, o->vx);
    TEST_ASSERT_EQUAL_FLOAT(-7.25f, o->vy);
    TEST_ASSERT_EQUAL_UINT32(0x12345678, o->color);
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
    cJSON *object = cJSON_GetArrayItem(cJSON_GetObjectItemCaseSensitive(json, "entities"), 0);
    for (int variant = 0; variant < 2; variant++) {
        cJSON_DeleteItemFromObjectCaseSensitive(object, "vx");
        cJSON_DeleteItemFromObjectCaseSensitive(object, "vy");
        if (variant) {
            cJSON_AddStringToObject(object, "vx", "invalid");
        }
        text = cJSON_Print(json);
        TEST_ASSERT_NOT_NULL(text);
        file = fopen(FILE_NAME, "wb");
        TEST_ASSERT_NOT_NULL(file);
        fputs(text, file);
        fclose(file);
        free(text);
        if (!variant) {
            TEST_ASSERT_TRUE(pe_scene_load(&scene, FILE_NAME, error, sizeof error));
            o = pe_entity_get(&scene, pe_entity_find(&scene, id));
            TEST_ASSERT_EQUAL_FLOAT(0, o->vx);
            TEST_ASSERT_EQUAL_FLOAT(0, o->vy);
        } else {
            original = scene;
            TEST_ASSERT_FALSE(pe_scene_load(&scene, FILE_NAME, error, sizeof error));
            TEST_ASSERT_EQUAL_MEMORY(&original, &scene, sizeof scene);
        }
    }
    cJSON_Delete(json);
    o = pe_entity_get(&scene, pe_entity_find(&scene, id));
    o->vx = NAN;
    TEST_ASSERT_FALSE(pe_scene_save(&scene, FILE_NAME, error, sizeof error));
    o->vx = 1000001;
    TEST_ASSERT_FALSE(pe_scene_save(&scene, FILE_NAME, error, sizeof error));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_add_defaults_and_dependencies);
    RUN_TEST(test_invalid_add_and_stale_handles_are_atomic);
    RUN_TEST(test_removal_protects_dependencies_and_hierarchy);
    RUN_TEST(test_readded_components_get_defaults);
    RUN_TEST(test_component_operations_undo_together);
    RUN_TEST(test_settings_roundtrip_and_optional_velocity);
    return UNITY_END();
}
