#include "editors/map/checkpoint.h"
#include "editors/scripts/checkpoint.h"
#include "unity.h"
#include <stdlib.h>
#include <string.h>

#define CACHE "editor-document-test.session"
static MapPayload *map_source, *map_target;
static PeScriptEditor *script_source, *script_target;
static char error[256];

void setUp(void) {
    *error = 0;
}

void tearDown(void) {
    if (map_source) {
        pe_history_free(&map_source->history);
        free(map_source);
    }
    if (map_target) {
        pe_history_free(&map_target->history);
        free(map_target);
    }
    if (script_source) {
        pe_text_free(&script_source->document);
        free(script_source);
    }
    if (script_target) {
        pe_text_free(&script_target->document);
        free(script_target);
    }
    map_source = map_target = NULL;
    script_source = script_target = NULL;
    remove(CACHE);
    remove(CACHE ".tmp");
}

static bool map_codec(PeSession *session, void *data) {
    return me_checkpoint(data, session);
}

static bool script_codec(PeSession *session, void *data) {
    return se_checkpoint(data, "project-root", NULL, session);
}

/* Links only document modules: no application singleton, window or UI event loop. */
static void test_map_checkpoint_preserves_independent_undo_and_redo(void) {
    map_source = calloc(1, sizeof *map_source);
    map_target = calloc(1, sizeof *map_target);
    TEST_ASSERT_NOT_NULL(map_source);
    TEST_ASSERT_NOT_NULL(map_target);
    pe_scene_init(&map_source->scene, 8, 8, 16);
    pe_history_init(&map_source->history);
    TEST_ASSERT_TRUE(pe_history_begin(&map_source->history, &map_source->scene));
    pe_tile_set(&map_source->scene, 0, 2, 3, 1);
    TEST_ASSERT_TRUE(pe_history_commit(&map_source->history, &map_source->scene));
    TEST_ASSERT_TRUE(pe_history_begin(&map_source->history, &map_source->scene));
    pe_tile_set(&map_source->scene, 0, 2, 3, 2);
    TEST_ASSERT_TRUE(pe_history_commit(&map_source->history, &map_source->scene));
    TEST_ASSERT_TRUE(pe_undo(&map_source->history, &map_source->scene));

    TEST_ASSERT_TRUE_MESSAGE(pe_session_save(CACHE, 1, map_codec, map_source, error, sizeof error),
                             error);
    TEST_ASSERT_TRUE_MESSAGE(pe_session_load(CACHE, 1, map_codec, map_target, error, sizeof error),
                             error);
    TEST_ASSERT_EQUAL_UINT16(1, pe_tile_get(&map_target->scene, 0, 2, 3));
    TEST_ASSERT_TRUE(pe_history_dirty(&map_target->history));
    TEST_ASSERT_TRUE(pe_redo(&map_target->history, &map_target->scene));
    TEST_ASSERT_EQUAL_UINT16(2, pe_tile_get(&map_target->scene, 0, 2, 3));
    TEST_ASSERT_EQUAL_UINT16(1, pe_tile_get(&map_source->scene, 0, 2, 3));
    TEST_ASSERT_TRUE(pe_undo(&map_target->history, &map_target->scene));
    TEST_ASSERT_TRUE(pe_undo(&map_target->history, &map_target->scene));
    TEST_ASSERT_EQUAL_UINT16(0, pe_tile_get(&map_target->scene, 0, 2, 3));
    TEST_ASSERT_FALSE(pe_history_dirty(&map_target->history));
}

static void test_script_checkpoint_preserves_utf8_history_and_view_state(void) {
    script_source = calloc(1, sizeof *script_source);
    script_target = calloc(1, sizeof *script_target);
    TEST_ASSERT_NOT_NULL(script_source);
    TEST_ASSERT_NOT_NULL(script_target);
    TEST_ASSERT_TRUE(pe_text_init(&script_source->document, "const name = '\xc3\xa4';\n"));
    script_source->document.cursor = script_source->document.anchor =
        script_source->document.length;
    TEST_ASSERT_TRUE(pe_text_insert(&script_source->document, "// first\n"));
    TEST_ASSERT_TRUE(pe_text_insert(&script_source->document, "// second\n"));
    TEST_ASSERT_TRUE(pe_text_undo(&script_source->document));
    strcpy(script_source->path, "scripts/player.js");
    script_source->exists = true;
    script_source->top_line = 2;
    script_source->left_column = 4;

    TEST_ASSERT_TRUE_MESSAGE(
        pe_session_save(CACHE, 2, script_codec, script_source, error, sizeof error), error);
    TEST_ASSERT_TRUE_MESSAGE(
        pe_session_load(CACHE, 2, script_codec, script_target, error, sizeof error), error);
    TEST_ASSERT_EQUAL_STRING(script_source->document.text, script_target->document.text);
    TEST_ASSERT_EQUAL(script_source->document.cursor, script_target->document.cursor);
    TEST_ASSERT_EQUAL_STRING("project-root", script_target->root);
    TEST_ASSERT_EQUAL_STRING("scripts/player.js", script_target->path);
    TEST_ASSERT_TRUE(script_target->exists);
    TEST_ASSERT_EQUAL_INT(2, script_target->top_line);
    TEST_ASSERT_EQUAL_INT(4, script_target->left_column);
    TEST_ASSERT_NULL(script_target->window);
    TEST_ASSERT_EQUAL_UINT64(UINT64_MAX, script_target->highlighted_revision);
    TEST_ASSERT_TRUE(pe_text_dirty(&script_target->document));
    TEST_ASSERT_TRUE(pe_text_redo(&script_target->document));
    TEST_ASSERT_NOT_NULL(strstr(script_target->document.text, "// second"));
    TEST_ASSERT_NULL(strstr(script_source->document.text, "// second"));
    TEST_ASSERT_TRUE(pe_text_undo(&script_target->document));
    TEST_ASSERT_TRUE(pe_text_undo(&script_target->document));
    TEST_ASSERT_FALSE(pe_text_dirty(&script_target->document));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_map_checkpoint_preserves_independent_undo_and_redo);
    RUN_TEST(test_script_checkpoint_preserves_utf8_history_and_view_state);
    return UNITY_END();
}
