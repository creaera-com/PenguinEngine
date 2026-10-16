#include "layout/ui_layout.h"
#include "unity.h"
#include "workspace/session.h"
#include <stdlib.h>
#include <string.h>
#define CACHE "editor-state-test.session"
#define YAML_FILE "editor-layout-test.yaml"
static const char *base =
    "schema: penguin.ui/v1\nkind: editor\nid: map\ndocument_provider: scene\ntoolbar: "
    "[]\ncolumns:\n  left: {type: spacer, id: left}\n  center: {type: spacer, id: center}\n  "
    "right: {type: spacer, id: right}\n";
static char error[2048];

static void write_file(const char *path, const char *text) {
    FILE *f = fopen(path, "wb");
    TEST_ASSERT_NOT_NULL(f);
    TEST_ASSERT_EQUAL(strlen(text), fwrite(text, 1, strlen(text), f));
    TEST_ASSERT_EQUAL(0, fclose(f));
}

void setUp(void) {
    *error = 0;
}

void tearDown(void) {
    remove(CACHE);
    remove(CACHE ".tmp");
    remove(YAML_FILE);
}

typedef struct {
    uint32_t value;
    char *text;
    bool called;
} Data;

static bool codec(PeSession *s, void *data) {
    Data *d = data;
    d->called = true;
    return pe_session_block(s, &d->value, sizeof d->value) && pe_session_string(s, &d->text, 200);
}

static bool broken(PeSession *s, void *data) {
    (void)data;
    uint32_t value = 99;
    pe_session_block(s, &value, sizeof value);
    return false;
}

static void test_session_roundtrip(void) {
    Data source = {123, "ungespeichert ä", false}, target = {0, NULL, false};
    TEST_ASSERT_TRUE(pe_session_save(CACHE, 7, codec, &source, error, sizeof error));
    TEST_ASSERT_TRUE(pe_session_load(CACHE, 7, codec, &target, error, sizeof error));
    TEST_ASSERT_EQUAL(123, target.value);
    TEST_ASSERT_EQUAL_STRING(source.text, target.text);
    free(target.text);
}

static void test_failed_save_keeps_checkpoint(void) {
    Data source = {123, "original", false}, target = {0, NULL, false};
    TEST_ASSERT_TRUE(pe_session_save(CACHE, 7, codec, &source, error, sizeof error));
    TEST_ASSERT_FALSE(pe_session_save(CACHE, 7, broken, &source, error, sizeof error));
    TEST_ASSERT_TRUE(pe_session_load(CACHE, 7, codec, &target, error, sizeof error));
    TEST_ASSERT_EQUAL_STRING("original", target.text);
    free(target.text);
}

static void test_checksum_before_decode(void) {
    Data source = {123, "original", false}, target = {0, NULL, false};
    TEST_ASSERT_TRUE(pe_session_save(CACHE, 7, codec, &source, error, sizeof error));
    FILE *f = fopen(CACHE, "r+b");
    TEST_ASSERT_NOT_NULL(f);
    fseek(f, -1, SEEK_END);
    fputc('X', f);
    fclose(f);
    TEST_ASSERT_FALSE(pe_session_load(CACHE, 7, codec, &target, error, sizeof error));
    TEST_ASSERT_FALSE(target.called);
}

static void test_truncated_session(void) {
    write_file(CACHE, "PES");
    Data target = {0, NULL, false};
    TEST_ASSERT_FALSE(pe_session_load(CACHE, 7, codec, &target, error, sizeof error));
    TEST_ASSERT_FALSE(target.called);
}

static void test_session_kind_and_trailing_bytes(void) {
    Data source = {123, "original", false}, target = {0, NULL, false};
    TEST_ASSERT_TRUE(pe_session_save(CACHE, 7, codec, &source, error, sizeof error));
    TEST_ASSERT_FALSE(pe_session_load(CACHE, 8, codec, &target, error, sizeof error));
    TEST_ASSERT_FALSE(target.called);
    FILE *f = fopen(CACHE, "ab");
    TEST_ASSERT_NOT_NULL(f);
    fputc('X', f);
    fclose(f);
    TEST_ASSERT_FALSE(pe_session_load(CACHE, 7, codec, &target, error, sizeof error));
    TEST_ASSERT_FALSE(target.called);
}

static void test_save_unwritable_destination(void) {
    Data source = {123, "original", false};
    TEST_ASSERT_FALSE(
        pe_session_save("missing-ui-session-dir/x", 7, codec, &source, error, sizeof error));
    TEST_ASSERT_NOT_EQUAL(0, *error);
}

static void rejects(const char *text, const char *fragment) {
    write_file(YAML_FILE, text);
    PeUiLayout *layout = pe_ui_layout_load(YAML_FILE, "editor", error, sizeof error);
    TEST_ASSERT_NULL(layout);
    TEST_ASSERT_NOT_NULL(strstr(error, YAML_FILE));
    if (fragment) {
        TEST_ASSERT_NOT_NULL(strstr(error, fragment));
    }
}

static void test_minimal_layout(void) {
    write_file(YAML_FILE, base);
    PeUiLayout *layout = pe_ui_layout_load(YAML_FILE, "editor", error, sizeof error);
    TEST_ASSERT_NOT_NULL_MESSAGE(layout, error);
    TEST_ASSERT_EQUAL_STRING("map", pe_ui_string(layout->root, "id", ""));
    pe_ui_layout_free(layout);
}

static void test_runtime_layouts(void) {
    const char *files[] = {PE_SOURCE_DIR "/resources/editor/ui/shell.yaml",
                           PE_SOURCE_DIR "/resources/editor/ui/editors/map.yaml",
                           PE_SOURCE_DIR "/resources/editor/ui/editors/scripts.yaml",
                           PE_SOURCE_DIR "/docs/examples/ui/shell.yaml",
                           PE_SOURCE_DIR "/docs/examples/ui/editors/map.yaml",
                           PE_SOURCE_DIR "/docs/examples/ui/editors/scripts.yaml"};
    for (int i = 0; i < 6; i++) {
        PeUiLayout *layout =
            pe_ui_layout_load(files[i], i % 3 ? "editor" : "shell", error, sizeof error);
        TEST_ASSERT_NOT_NULL_MESSAGE(layout, error);
        pe_ui_layout_free(layout);
    }
}

static void test_unknown_widget_and_line(void) {
    rejects("schema: penguin.ui/v1\nkind: editor\nid: map\ndocument_provider: scene\ntoolbar: "
            "[]\ncolumns:\n  left: {type: filetree, id: left}\n  center: {type: spacer, id: "
            "center}\n  right: {type: spacer, id: right}\n",
            "unknown widget");
    TEST_ASSERT_NOT_NULL(strstr(error, ":7:"));
}

static void test_duplicate_keys(void) {
    rejects("schema: penguin.ui/v1\nkind: editor\nid: map\nid: scripts\n", "duplicate");
}

static void test_duplicate_ids(void) {
    rejects("schema: penguin.ui/v1\nkind: editor\nid: map\ndocument_provider: scene\ntoolbar: "
            "[]\ncolumns:\n  left: {type: spacer, id: same}\n  center: {type: spacer, id: same}\n  "
            "right: {type: spacer, id: right}\n",
            "duplicate id");
}

static void test_unknown_action(void) {
    rejects("schema: penguin.ui/v1\nkind: editor\nid: map\ndocument_provider: scene\ntoolbar: "
            "[{type: button, id: action, action: map.nonexistent}]\ncolumns: {}\n",
            "unknown action");
}

static void test_wrong_binding_type(void) {
    rejects("schema: penguin.ui/v1\nkind: editor\nid: map\ndocument_provider: scene\ntoolbar: "
            "[{type: button, id: action, enabled: {binding: scene.active}}]\ncolumns: {}\n",
            "Boolean binding");
}

static void test_unknown_property(void) {
    rejects("schema: penguin.ui/v1\nkind: editor\nid: map\ndocument_provider: scene\ntoolbar: "
            "[{type: button, id: action, children: []}]\ncolumns: {}\n",
            "unknown field");
}

static void test_unsupported_yaml_features(void) {
    rejects("x: &anchor {id: hello}\ny: *anchor\n", "unsupported");
    rejects("x: !!str value\n", "unsupported");
    rejects("x: a\n---\nx: b\n", "unsupported");
}

static void test_module_mismatch(void) {
    rejects("schema: penguin.ui/v1\nkind: editor\nid: scripts\ndocument_provider: script\ntoolbar: "
            "[]\ncolumns:\n  left: {type: spacer, id: left}\n  center: {type: map_canvas, id: "
            "center, binding: scene.active}\n  right: {type: spacer, id: right}\n",
            "unavailable");
}

static void test_widget_binding_mismatch(void) {
    rejects("schema: penguin.ui/v1\nkind: editor\nid: map\ndocument_provider: scene\ntoolbar: "
            "[]\ncolumns:\n  left: {type: spacer, id: left}\n  center: {type: map_canvas, id: "
            "center, binding: document.active}\n  right: {type: spacer, id: right}\n",
            "incompatible");
}

static void test_depth_and_size_limits(void) {
    char nested[200];
    int n = 0;
    for (int i = 0; i < 40; i++) {
        nested[n++] = '[';
    }
    nested[n++] = '0';
    for (int i = 0; i < 40; i++) {
        nested[n++] = ']';
    }
    nested[n] = 0;
    rejects(nested, "limits exceeded");
    FILE *f = fopen(YAML_FILE, "wb");
    TEST_ASSERT_NOT_NULL(f);
    for (int i = 0; i <= PE_UI_MAX_FILE; i++) {
        fputc('x', f);
    }
    fclose(f);
    TEST_ASSERT_NULL(pe_ui_layout_load(YAML_FILE, "editor", error, sizeof error));
    TEST_ASSERT_NOT_NULL(strstr(error, "size limit"));
}

static void test_missing_layout(void) {
    TEST_ASSERT_NULL(pe_ui_layout_load("missing-ui-file.yaml", "editor", error, sizeof error));
    TEST_ASSERT_NOT_NULL(strstr(error, "cannot open"));
}

static void test_duplicate_specialized_widgets(void) {
    rejects("schema: penguin.ui/v1\nkind: editor\nid: map\ndocument_provider: scene\ntoolbar: "
            "[]\ncolumns:\n  left: {type: file_tree, id: left, binding: project.files}\n  center: "
            "{type: spacer, id: center}\n  right: {type: file_tree, id: right, binding: "
            "project.files}\n",
            "only once");
}

static void test_selector_must_be_last(void) {
    PeUiLayout *shell = pe_ui_layout_load(PE_SOURCE_DIR "/resources/editor/ui/shell.yaml", "shell",
                                          error, sizeof error);
    TEST_ASSERT_NOT_NULL(shell);
    cJSON *button = cJSON_CreateObject();
    cJSON_AddStringToObject(button, "type", "button");
    cJSON_AddStringToObject(button, "id", "after_selector");
    cJSON_AddItemToArray((cJSON *)pe_ui_get(shell->root, "toolbar"), button);
    char *text = cJSON_PrintUnformatted(shell->root);
    TEST_ASSERT_NOT_NULL(text);
    write_file(YAML_FILE, text);
    free(text);
    pe_ui_layout_free(shell);
    TEST_ASSERT_NULL(pe_ui_layout_load(YAML_FILE, "shell", error, sizeof error));
    TEST_ASSERT_NOT_NULL(strstr(error, "last toolbar element"));
    shell = pe_ui_layout_load(PE_SOURCE_DIR "/resources/editor/ui/shell.yaml", "shell", error,
                              sizeof error);
    TEST_ASSERT_NOT_NULL(shell);
    cJSON *control = cJSON_GetArrayItem(pe_ui_get(shell->root, "window_controls"), 0);
    TEST_ASSERT_NOT_NULL(control);
    cJSON_ReplaceItemInObjectCaseSensitive(control, "action", cJSON_CreateString("document.save"));
    text = cJSON_PrintUnformatted(shell->root);
    TEST_ASSERT_NOT_NULL(text);
    write_file(YAML_FILE, text);
    free(text);
    pe_ui_layout_free(shell);
    TEST_ASSERT_NULL(pe_ui_layout_load(YAML_FILE, "shell", error, sizeof error));
    TEST_ASSERT_NOT_NULL(strstr(error, "window action buttons"));
}

static void test_split_and_terminal_schema(void) {
    const char *valid =
        "schema: penguin.ui/v1\nkind: editor\nid: map\ndocument_provider: scene\ntoolbar: "
        "[]\ncolumns:\n"
        "  left: {type: spacer, id: left}\n  right: {type: spacer, id: right}\n  center:\n"
        "    type: split\n    id: split\n    axis: vertical\n    initial: 210\n    minimum_first: "
        "160\n"
        "    minimum_second: 100\n    gap: 8\n    first: {type: map_canvas, id: map_canvas, "
        "binding: "
        "scene.active}\n"
        "    second: {type: terminal, id: console, binding: terminal.shared}\n";
    write_file(YAML_FILE, valid);
    PeUiLayout *layout = pe_ui_layout_load(YAML_FILE, "editor", error, sizeof error);
    TEST_ASSERT_NOT_NULL_MESSAGE(layout, error);
    pe_ui_layout_free(layout);
    char *invalid = malloc(strlen(valid) + 1);
    TEST_ASSERT_NOT_NULL(invalid);
    strcpy(invalid, valid);
    char *axis = strstr(invalid, "vertical");
    memcpy(axis, "diagonal", 8);
    rejects(invalid, "vertical or horizontal");
    free(invalid);
}

static void test_terminal_binding_and_uniqueness(void) {
    rejects("schema: penguin.ui/v1\nkind: editor\nid: map\ndocument_provider: scene\ntoolbar: "
            "[]\ncolumns:\n"
            "  left: {type: terminal, id: first, binding: terminal.shared}\n"
            "  right: {type: terminal, id: second, binding: terminal.shared}\n"
            "  center: {type: map_canvas, id: map_canvas, binding: scene.active}\n",
            "only once");
    rejects("schema: penguin.ui/v1\nkind: editor\nid: scripts\ndocument_provider: script\ntoolbar: "
            "[]\ncolumns:\n"
            "  left: {type: spacer, id: left}\n  right: {type: spacer, id: right}\n"
            "  center: {type: terminal, id: console, binding: document.active}\n",
            "incompatible");
}

static void test_scene_tree_tools_schema(void) {
    const char *valid = "schema: penguin.ui/v1\nkind: editor\nid: map\ndocument_provider: "
                        "scene\ntoolbar: []\ncolumns:\n"
                        "  left:\n    type: entity_tree\n    id: entities\n    binding: "
                        "scene.entities\n    tools:\n"
                        "      - {type: button, id: add, icon: plus, action: scene.add_child}\n"
                        "  center: {type: map_canvas, id: canvas, binding: scene.active}\n"
                        "  right: {type: spacer, id: empty}\n";
    write_file(YAML_FILE, valid);
    PeUiLayout *layout = pe_ui_layout_load(YAML_FILE, "editor", error, sizeof error);
    TEST_ASSERT_NOT_NULL_MESSAGE(layout, error);
    pe_ui_layout_free(layout);
    char *invalid = malloc(strlen(valid) + 1);
    TEST_ASSERT_NOT_NULL(invalid);
    strcpy(invalid, valid);
    char *button = strstr(invalid, "type: button");
    memcpy(button + 6, "spacer", 6);
    rejects(invalid, "only supports buttons");
    free(invalid);
}

static void test_map_panel_widgets_are_typed_and_unique(void) {
    rejects("schema: penguin.ui/v1\nkind: editor\nid: map\ndocument_provider: scene\ntoolbar: "
            "[]\ncolumns:\n"
            "  left: {type: map_layers, id: layers, binding: selection.active}\n"
            "  center: {type: map_canvas, id: canvas, binding: scene.active}\n"
            "  right: {type: tile_palette, id: tiles, binding: scene.active}\n",
            "incompatible");
    rejects("schema: penguin.ui/v1\nkind: editor\nid: map\ndocument_provider: scene\ntoolbar: "
            "[]\ncolumns:\n"
            "  left: {type: tile_palette, id: first, binding: scene.active}\n"
            "  center: {type: map_canvas, id: canvas, binding: scene.active}\n"
            "  right: {type: tile_palette, id: second, binding: scene.active}\n",
            "only once");
    rejects("schema: penguin.ui/v1\nkind: editor\nid: scripts\ndocument_provider: script\ntoolbar: "
            "[]\ncolumns:\n"
            "  left: {type: map_layers, id: layers, binding: scene.active}\n"
            "  center: {type: script_editor, id: document, binding: document.active}\n"
            "  right: {type: spacer, id: empty}\n",
            "unavailable");
}

static void test_tool_panel_schema(void) {
    const char *valid = "schema: penguin.ui/v1\nkind: editor\nid: map\ndocument_provider: "
                        "scene\ntoolbar: []\ncolumns:\n"
                        "  left: {type: spacer, id: left}\n"
                        "  center: {type: map_canvas, id: canvas, binding: scene.active}\n"
                        "  right:\n    type: tool_panel\n    id: tools\n    children:\n"
                        "      - type: section\n        id: group\n        title: Tools\n        "
                        "open: true\n        children:\n"
                        "          - {type: button, id: action}\n";
    write_file(YAML_FILE, valid);
    PeUiLayout *layout = pe_ui_layout_load(YAML_FILE, "editor", error, sizeof error);
    TEST_ASSERT_NOT_NULL_MESSAGE(layout, error);
    pe_ui_layout_free(layout);
    char *invalid = malloc(strlen(valid) + 1);
    TEST_ASSERT_NOT_NULL(invalid);
    strcpy(invalid, valid);
    memcpy(strstr(invalid, "type: button") + 6, "spacer", 6);
    rejects(invalid, "only support buttons");
    free(invalid);
    const char *files[] = {PE_SOURCE_DIR "/resources/editor/ui/editors/vector.yaml",
                           PE_SOURCE_DIR "/docs/examples/ui/editors/vector.yaml"};
    for (int i = 0; i < 2; i++) {
        layout = pe_ui_layout_load(files[i], "editor", error, sizeof error);
        TEST_ASSERT_NOT_NULL_MESSAGE(layout, error);
        pe_ui_layout_free(layout);
    }
}

static void test_vector_preview_schema(void) {
    const char *valid =
        "schema: penguin.ui/v1\nkind: editor\nid: vector\ndocument_provider: vector\n"
        "toolbar: []\ncolumns:\n  left: {type: spacer, id: left}\n"
        "  center: {type: vector_canvas, id: canvas, binding: vector.active}\n"
        "  right: {type: vector_preview, id: preview, binding: vector.active, height: 190}\n";
    write_file(YAML_FILE, valid);
    PeUiLayout *layout = pe_ui_layout_load(YAML_FILE, "editor", error, sizeof error);
    TEST_ASSERT_NOT_NULL_MESSAGE(layout, error);
    pe_ui_layout_free(layout);
    char *invalid = malloc(strlen(valid) + 1);
    TEST_ASSERT_NOT_NULL(invalid);
    strcpy(invalid, valid);
    memcpy(strstr(invalid, "height: 190") + 8, "000", 3);
    rejects(invalid, "range");
    free(invalid);
    rejects("schema: penguin.ui/v1\nkind: editor\nid: map\ndocument_provider: scene\n"
            "toolbar: []\ncolumns:\n  left: {type: spacer, id: left}\n"
            "  center: {type: map_canvas, id: canvas, binding: scene.active}\n"
            "  right: {type: vector_preview, id: preview, binding: vector.active, height: 190}\n",
            "unavailable");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_vector_preview_schema);
    RUN_TEST(test_tool_panel_schema);
    RUN_TEST(test_map_panel_widgets_are_typed_and_unique);
    RUN_TEST(test_scene_tree_tools_schema);
    RUN_TEST(test_split_and_terminal_schema);
    RUN_TEST(test_terminal_binding_and_uniqueness);
    RUN_TEST(test_session_roundtrip);
    RUN_TEST(test_failed_save_keeps_checkpoint);
    RUN_TEST(test_checksum_before_decode);
    RUN_TEST(test_truncated_session);
    RUN_TEST(test_session_kind_and_trailing_bytes);
    RUN_TEST(test_save_unwritable_destination);
    RUN_TEST(test_minimal_layout);
    RUN_TEST(test_runtime_layouts);
    RUN_TEST(test_unknown_widget_and_line);
    RUN_TEST(test_duplicate_keys);
    RUN_TEST(test_duplicate_ids);
    RUN_TEST(test_unknown_action);
    RUN_TEST(test_wrong_binding_type);
    RUN_TEST(test_unknown_property);
    RUN_TEST(test_unsupported_yaml_features);
    RUN_TEST(test_module_mismatch);
    RUN_TEST(test_widget_binding_mismatch);
    RUN_TEST(test_depth_and_size_limits);
    RUN_TEST(test_missing_layout);
    RUN_TEST(test_duplicate_specialized_widgets);
    RUN_TEST(test_selector_must_be_last);
    return UNITY_END();
}
