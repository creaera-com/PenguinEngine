#include "pengui/pengui.h"
#include "unity.h"
#include <stdio.h>
#include <string.h>

void setUp(void) {}

void tearDown(void) {}

static PUILayout *parse(const char *body) {
    char error[256];
    PUILayout *layout = pui_layout_parse(body, strlen(body), error, sizeof error);
    TEST_ASSERT_NOT_NULL_MESSAGE(layout, error);
    return layout;
}

static void test_composition_copies_inputs_and_regenerates_automatic_ids(void) {
    PUILayout *base = parse("schema: pengui/v1\nroot: {type: column, children: ["
                            "{type: label, id: outer}, {type: slot, id: body}]}");
    PUILayout *child =
        parse("schema: pengui/v1\nroot: {type: panel, children: [{type: input, id: inner}]}");
    char error[256];
    PUILayout *joined = pui_layout_compose(base, "body", child, error, sizeof error);
    TEST_ASSERT_NOT_NULL_MESSAGE(joined, error);
    TEST_ASSERT_EQUAL_INT(base->count + child->count, joined->count);
    TEST_ASSERT_EQUAL_INT(-1, base->nodes[2].child);
    TEST_ASSERT_EQUAL_INT(2, child->count);
    for (int i = 0; i < joined->count; i++) {
        for (int j = 0; j < i; j++) {
            TEST_ASSERT_NOT_EQUAL(joined->nodes[i].id, joined->nodes[j].id);
        }
    }
    pui_layout_free(base);
    pui_layout_free(child);
    TEST_ASSERT_EQUAL_STRING("inner", joined->nodes[4].name);
    PUILayout *empty = pui_layout_compose(joined, "body", NULL, error, sizeof error);
    TEST_ASSERT_NOT_NULL_MESSAGE(empty, error);
    TEST_ASSERT_EQUAL_INT(3, empty->count);
    pui_layout_free(empty);
    pui_layout_free(joined);
}

static void test_duplicate_ids_missing_slot_and_capacity_fail_atomically(void) {
    PUILayout *base = parse("schema: pengui/v1\nroot: {type: column, children: ["
                            "{type: label, id: taken}, {type: slot, id: body}]}");
    PUILayout *child = parse("schema: pengui/v1\nroot: {type: input, id: taken}");
    char error[256];
    TEST_ASSERT_NULL(pui_layout_compose(base, "missing", child, error, sizeof error));
    TEST_ASSERT_NULL(pui_layout_compose(base, "body", child, error, sizeof error));
    TEST_ASSERT_EQUAL_INT(3, base->count);
    pui_layout_free(child);
    char yaml[128 + PUI_LAYOUT_NODES * sizeof(",{type: label}")] =
        "schema: pengui/v1\nroot: {type: column, children: [";
    for (int i = 0; i < PUI_LAYOUT_NODES - 2; i++) {
        strcat(yaml, i ? ",{type: label}" : "{type: label}");
    }
    strcat(yaml, "]}");
    child = parse(yaml);
    TEST_ASSERT_NULL(pui_layout_compose(base, "body", child, error, sizeof error));
    TEST_ASSERT_NOT_NULL(strstr(error, "limit"));
    pui_layout_free(child);
    pui_layout_free(base);
}

static void test_image_paths_keep_each_source_directory(void) {
    PUILayout *base = parse("schema: pengui/v1\nroot: {type: column, children: ["
                            "{type: button, image: icons/a.png}, {type: slot, id: body}]}");
    PUILayout *child = parse("schema: pengui/v1\nroot: {type: button, image: b.png}");
    snprintf(base->directory, sizeof base->directory, "/shell/");
    snprintf(child->directory, sizeof child->directory, "/editor/");
    char error[256];
    PUILayout *joined = pui_layout_compose(base, "body", child, error, sizeof error);
    TEST_ASSERT_NOT_NULL_MESSAGE(joined, error);
    TEST_ASSERT_EQUAL_STRING("/shell/icons/a.png", joined->nodes[1].image);
    TEST_ASSERT_EQUAL_STRING("/editor/b.png", joined->nodes[3].image);
    PUILayout *again = pui_layout_compose(joined, "body", child, error, sizeof error);
    TEST_ASSERT_NOT_NULL_MESSAGE(again, error);
    TEST_ASSERT_EQUAL_STRING("/shell/icons/a.png", again->nodes[1].image);
    pui_layout_free(again);
    pui_layout_free(joined);
    pui_layout_free(child);
    pui_layout_free(base);
}

static void test_dynamic_tabs_inherit_theme_shape_and_transition(void) {
    PUILayout *base = parse("schema: pengui/v1\nroot: {type: tabs, id: documents, children: []}");
    PUILayout *content = parse("schema: pengui/v1\nroot: {type: canvas}");
    char error[256];
    PUILayout *joined = pui_layout_add_tab(base, "documents", "document_1", "Document 1", content,
                                           error, sizeof error);
    TEST_ASSERT_NOT_NULL_MESSAGE(joined, error);
    int tab = joined->nodes[joined->root].child;
    TEST_ASSERT_TRUE(tab >= 0);
    TEST_ASSERT_EQUAL_FLOAT(-1, joined->nodes[tab].radius);
    TEST_ASSERT_EQUAL_FLOAT(-1, joined->nodes[tab].transition_ms);
    pui_layout_free(joined);
    pui_layout_free(content);
    pui_layout_free(base);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_dynamic_tabs_inherit_theme_shape_and_transition);
    RUN_TEST(test_composition_copies_inputs_and_regenerates_automatic_ids);
    RUN_TEST(test_duplicate_ids_missing_slot_and_capacity_fail_atomically);
    RUN_TEST(test_image_paths_keep_each_source_directory);
    return UNITY_END();
}
