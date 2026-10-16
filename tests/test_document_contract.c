#include "editor/document/document.h"
#include "unity.h"
#include <string.h>

typedef struct {
    PeDocumentStatus status;
    int saves;
    int undos;
    bool redo;
} FakeDocument;

static FakeDocument first, second;

static PeDocumentStatus status(void *context) {
    return ((FakeDocument *)context)->status;
}

static bool save(void *context) {
    ((FakeDocument *)context)->saves++;
    return true;
}

static void undo(void *context, bool redo) {
    FakeDocument *fake = context;
    fake->undos++;
    fake->redo = redo;
}

static void describe(void *context, char *buffer, size_t size) {
    (void)context;
    memset(buffer, 'x', size);
}

static const PeDocumentOps ops = {
    .status = status, .save = save, .undo = undo, .describe = describe};

void setUp(void) {
    memset(&first, 0, sizeof first);
    memset(&second, 0, sizeof second);
}

void tearDown(void) {}

static void test_empty_binding_is_safe(void) {
    PeDocument empty = {0};
    TEST_ASSERT_FALSE(pe_document_status(empty).loaded);
    TEST_ASSERT_FALSE(pe_document_save(empty));
    pe_document_undo(empty, false);
    char text[8] = "before";
    pe_document_describe(empty, text, sizeof text);
    TEST_ASSERT_EQUAL_STRING("", text);
    TEST_ASSERT_FALSE(pe_document_save((PeDocument){&first, NULL}));
    TEST_ASSERT_FALSE(pe_document_save((PeDocument){NULL, &ops}));
}

static void test_save_requires_loaded_finished_document(void) {
    PeDocument document = {&first, &ops};
    TEST_ASSERT_FALSE(pe_document_save(document));
    first.status.loaded = true;
    first.status.pending = true;
    TEST_ASSERT_FALSE(pe_document_save(document));
    first.status.pending = false;
    TEST_ASSERT_TRUE(pe_document_save(document));
    TEST_ASSERT_EQUAL_INT(1, first.saves);
    PeDocumentOps incomplete = {.status = status};
    TEST_ASSERT_FALSE(pe_document_save((PeDocument){&first, &incomplete}));
}

static void test_undo_redo_checks_capabilities_and_pending_gestures(void) {
    PeDocument document = {&first, &ops};
    first.status.loaded = true;
    first.status.can_undo = true;
    pe_document_undo(document, true);
    TEST_ASSERT_EQUAL_INT(0, first.undos);
    pe_document_undo(document, false);
    TEST_ASSERT_EQUAL_INT(1, first.undos);
    TEST_ASSERT_FALSE(first.redo);
    first.status.can_redo = true;
    pe_document_undo(document, true);
    TEST_ASSERT_EQUAL_INT(2, first.undos);
    TEST_ASSERT_TRUE(first.redo);
    first.status.pending = true;
    pe_document_undo(document, false);
    TEST_ASSERT_EQUAL_INT(2, first.undos);
}

static void test_description_is_bounded_and_terminated(void) {
    char text[5] = "abcd";
    PeDocument document = {&first, &ops};
    pe_document_describe(document, text, sizeof text);
    TEST_ASSERT_EQUAL_STRING("xxxx", text);
    pe_document_describe(document, text, 1);
    TEST_ASSERT_EQUAL_STRING("", text);
    pe_document_describe(document, NULL, 0);
    pe_document_describe(document, text, 0);
}

static void test_bindings_preserve_independent_contexts(void) {
    first.status = (PeDocumentStatus){.loaded = true, .dirty = true, .can_undo = true};
    second.status = (PeDocumentStatus){.loaded = true, .exists = true, .can_redo = true};
    PeDocument a = {&first, &ops}, b = {&second, &ops};
    TEST_ASSERT_TRUE(pe_document_status(a).dirty);
    TEST_ASSERT_FALSE(pe_document_status(b).dirty);
    TEST_ASSERT_TRUE(pe_document_status(b).exists);
    TEST_ASSERT_TRUE(pe_document_save(a));
    pe_document_undo(b, true);
    TEST_ASSERT_EQUAL_INT(1, first.saves);
    TEST_ASSERT_EQUAL_INT(0, second.saves);
    TEST_ASSERT_EQUAL_INT(0, first.undos);
    TEST_ASSERT_EQUAL_INT(1, second.undos);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_empty_binding_is_safe);
    RUN_TEST(test_save_requires_loaded_finished_document);
    RUN_TEST(test_undo_redo_checks_capabilities_and_pending_gestures);
    RUN_TEST(test_description_is_bounded_and_terminated);
    RUN_TEST(test_bindings_preserve_independent_contexts);
    return UNITY_END();
}
