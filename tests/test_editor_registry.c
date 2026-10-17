#include "editor/registry.h"
#include "unity.h"

static const EditorView dialogue = {.id = 91, .module = "dialogue", .open_request = 701};
static const EditorView animation = {.id = 12,
                                     .module = "animation",
                                     .open_request = 902,
                                     .new_request = 903,
                                     .import_request = 904};
static const EditorView *const views[] = {&animation, &dialogue};
static const PeEditorRegistry registry = {views, 2};

void setUp(void) {}

void tearDown(void) {}

static void test_ids_do_not_depend_on_order_or_builtin_editors(void) {
    TEST_ASSERT_EQUAL_PTR(&dialogue, pe_editor_registry_find(registry, 91));
    TEST_ASSERT_EQUAL_PTR(&animation, pe_editor_registry_find(registry, 12));
    TEST_ASSERT_EQUAL_INT(1, pe_editor_registry_index(registry, 91));
    TEST_ASSERT_NULL(pe_editor_registry_find(registry, 999));
    TEST_ASSERT_EQUAL_INT(-1, pe_editor_registry_index(registry, 999));
}

static void test_module_names_route_without_builtin_switches(void) {
    TEST_ASSERT_EQUAL_PTR(&dialogue, pe_editor_registry_module(registry, "dialogue"));
    TEST_ASSERT_NULL(pe_editor_registry_module(registry, "map"));
    TEST_ASSERT_NULL(pe_editor_registry_module(registry, NULL));
}

static void test_open_new_and_import_find_the_registered_owner(void) {
    TEST_ASSERT_EQUAL_PTR(&dialogue, pe_editor_registry_request(registry, 701));
    TEST_ASSERT_EQUAL_PTR(&animation, pe_editor_registry_request(registry, 902));
    TEST_ASSERT_EQUAL_PTR(&animation, pe_editor_registry_request(registry, 903));
    TEST_ASSERT_EQUAL_PTR(&animation, pe_editor_registry_request(registry, 904));
    TEST_ASSERT_NULL(pe_editor_registry_request(registry, 0));
    TEST_ASSERT_NULL(pe_editor_registry_request(registry, 999));
}

static void test_empty_registry_has_no_owner(void) {
    PeEditorRegistry empty = {0};
    TEST_ASSERT_NULL(pe_editor_registry_find(empty, 91));
    TEST_ASSERT_NULL(pe_editor_registry_module(empty, "dialogue"));
    TEST_ASSERT_NULL(pe_editor_registry_request(empty, 701));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_ids_do_not_depend_on_order_or_builtin_editors);
    RUN_TEST(test_module_names_route_without_builtin_switches);
    RUN_TEST(test_open_new_and_import_find_the_registered_owner);
    RUN_TEST(test_empty_registry_has_no_owner);
    return UNITY_END();
}
