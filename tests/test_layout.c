#include "pengui/pengui.h"
#include "unity.h"
#include <stdio.h>
#include <string.h>

void setUp(void) {}

void tearDown(void) {}

static PUILayout *parse(const char *body, char *error) {
    return pui_layout_parse(body, strlen(body), error, 256);
}

static void test_actual_playground_is_generic_and_bounded(void) {
    char error[256];
    PUILayout *layout = pui_layout_load(PENGUI_LAYOUT_FILE, error, sizeof error);
    TEST_ASSERT_NOT_NULL_MESSAGE(layout, error);
    TEST_ASSERT_EQUAL_INT(1280, layout->width);
    TEST_ASSERT_TRUE(layout->count > 30 && layout->count < PUI_LAYOUT_NODES);
    TEST_ASSERT_EQUAL_INT(PUI_NODE_COLUMN, layout->nodes[layout->root].type);
    pui_layout_free(layout);
}

static void test_duplicate_ids_and_properties_are_rejected(void) {
    char error[256];
    TEST_ASSERT_NULL(parse("schema: pengui/v1\nroot: {type: row, children: [{type: button, id: "
                           "same}, {type: button, id: same}]}",
                           error));
    TEST_ASSERT_NOT_NULL(strstr(error, "duplicate widget id"));
    TEST_ASSERT_NULL(
        parse("schema: pengui/v1\nroot: {type: button, text: first, text: second}", error));
    TEST_ASSERT_NOT_NULL(strstr(error, "duplicate property"));
}

static void test_unknown_schema_widgets_and_keys_are_rejected(void) {
    char error[256];
    TEST_ASSERT_NULL(parse("schema: penguin.ui/v1\nroot: {type: label}", error));
    TEST_ASSERT_NULL(parse("schema: pengui/v1\nroot: {type: map_canvas}", error));
    TEST_ASSERT_NULL(parse("schema: pengui/v1\nroot: {type: label, widht: 30}", error));
    TEST_ASSERT_NOT_NULL(strstr(error, "unknown property"));
}

static void test_invalid_numbers_booleans_and_colors_are_rejected(void) {
    char error[256];
    TEST_ASSERT_NULL(parse("schema: pengui/v1\nroot: {type: label, width: nan}", error));
    TEST_ASSERT_NULL(parse("schema: pengui/v1\nroot: {type: label, enabled: maybe}", error));
    TEST_ASSERT_NULL(
        parse("schema: pengui/v1\ntheme: {accent: '#GG0000'}\nroot: {type: label}", error));
    TEST_ASSERT_NULL(parse("schema: pengui/v1\nroot: {type: button, image_padding: -1}", error));
}

static void test_child_structure_is_validated(void) {
    char error[256];
    TEST_ASSERT_NULL(
        parse("schema: pengui/v1\nroot: {type: split, children: [{type: label}]}", error));
    TEST_ASSERT_NULL(
        parse("schema: pengui/v1\nroot: {type: tabs, children: [{type: button}]}", error));
    TEST_ASSERT_NULL(
        parse("schema: pengui/v1\nroot: {type: button, children: [{type: label}]}", error));
    TEST_ASSERT_NULL(
        parse("schema: pengui/v1\nroot: {type: tree, children: [{type: label}]}", error));
}

static void test_alias_cycles_and_node_limits_are_bounded(void) {
    char error[256];
    TEST_ASSERT_NULL(parse("schema: pengui/v1\nroot: &loop {type: row, children: [*loop]}", error));
    char yaml[12000] = "schema: pengui/v1\nroot:\n  type: column\n  children:\n";
    for (int i = 0; i < PUI_LAYOUT_NODES + 1; i++) {
        strcat(yaml, "    - {type: label}\n");
    }
    TEST_ASSERT_NULL(parse(yaml, error));
    TEST_ASSERT_NOT_NULL(strstr(error, "limit"));
}

static void test_empty_multiple_and_broken_documents_are_rejected(void) {
    char error[256];
    TEST_ASSERT_NULL(parse("", error));
    TEST_ASSERT_NULL(parse("schema: pengui/v1\nroot: [broken", error));
    TEST_ASSERT_NULL(parse("schema: pengui/v1\nroot: {type: label}\n---\nextra: document", error));
    TEST_ASSERT_NULL(pui_layout_load("missing-layout-file.yaml", error, sizeof error));
}

static void test_theme_and_defaults_are_read_without_sdl(void) {
    char error[256];
    PUILayout *layout = parse(
        "schema: pengui/v1\ntheme: {accent: '#112233'}\nroot: {type: input, id: name, text: Hello}",
        error);
    TEST_ASSERT_NOT_NULL_MESSAGE(layout, error);
    TEST_ASSERT_EQUAL_HEX32(0x112233ff, layout->colors[4]);
    TEST_ASSERT_EQUAL_STRING("Hello", layout->nodes[0].text);
    TEST_ASSERT_TRUE(layout->nodes[0].enabled);
    pui_layout_free(layout);
}

static void test_split_child_count_and_flexible_pane_are_validated(void) {
    char error[256];
    const char *yaml =
        "schema: pengui/v1\nroot: {type: split, flex: middle, initial: 200, initial_last: 250, "
        "children: [{type: panel}, {type: panel}, {type: panel}]}";
    PUILayout *layout = parse(yaml, error);
    TEST_ASSERT_NOT_NULL_MESSAGE(layout, error);
    TEST_ASSERT_EQUAL_INT(1, layout->nodes[0].flexible);
    TEST_ASSERT_EQUAL_FLOAT(250, layout->nodes[0].initial_last);
    pui_layout_free(layout);
    TEST_ASSERT_NULL(parse("schema: pengui/v1\nroot: {type: split, children: [{type: panel}, "
                           "{type: panel}, {type: panel}, {type: panel}]}",
                           error));
    TEST_ASSERT_NULL(parse("schema: pengui/v1\nroot: {type: split, flex: middle, children: [{type: "
                           "panel}, {type: panel}]}",
                           error));
    layout = parse("schema: pengui/v1\nroot: {type: split, flex: first, initial_last: 180, "
                   "children: [{type: panel}, {type: panel}]}",
                   error);
    TEST_ASSERT_NOT_NULL_MESSAGE(layout, error);
    TEST_ASSERT_EQUAL_INT(0, layout->nodes[0].flexible);
    pui_layout_free(layout);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_split_child_count_and_flexible_pane_are_validated);
    RUN_TEST(test_actual_playground_is_generic_and_bounded);
    RUN_TEST(test_duplicate_ids_and_properties_are_rejected);
    RUN_TEST(test_unknown_schema_widgets_and_keys_are_rejected);
    RUN_TEST(test_invalid_numbers_booleans_and_colors_are_rejected);
    RUN_TEST(test_child_structure_is_validated);
    RUN_TEST(test_alias_cycles_and_node_limits_are_bounded);
    RUN_TEST(test_empty_multiple_and_broken_documents_are_rejected);
    RUN_TEST(test_theme_and_defaults_are_read_without_sdl);
    return UNITY_END();
}
