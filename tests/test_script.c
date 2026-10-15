#include "penguin/script.h"
#include "unity.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static PeText doc;
static char error[256];

void setUp(void) {
    TEST_ASSERT_TRUE(pe_text_init(&doc, ""));
    error[0] = 0;
}

void tearDown(void) {
    pe_text_free(&doc);
    remove("test_script.js");
    remove("test_script.js.tmp");
}

static void source(const char *s) {
    pe_text_free(&doc);
    TEST_ASSERT_TRUE(pe_text_init(&doc, s));
}

static void test_insert_and_selection(void) {
    pe_text_insert(&doc, "hello world");
    doc.anchor = 6;
    doc.cursor = 11;
    TEST_ASSERT_TRUE(pe_text_insert(&doc, "Penguin"));
    TEST_ASSERT_EQUAL_STRING("hello Penguin", doc.text);
    TEST_ASSERT_EQUAL_UINT(13, doc.cursor);
    TEST_ASSERT_EQUAL_UINT(doc.cursor, doc.anchor);
}

static void test_utf8_backspace(void) {
    pe_text_insert(&doc, "a\xc3\xa4\xf0\x9f\x90\xa7");
    TEST_ASSERT_TRUE(pe_text_backspace(&doc));
    TEST_ASSERT_EQUAL_STRING("a\xc3\xa4", doc.text);
    TEST_ASSERT_TRUE(pe_text_backspace(&doc));
    TEST_ASSERT_EQUAL_STRING("a", doc.text);
}

static void test_utf8_delete(void) {
    source("\xc3\xa4z");
    TEST_ASSERT_TRUE(pe_text_delete(&doc));
    TEST_ASSERT_EQUAL_STRING("z", doc.text);
}

static void test_invalid_utf8_atomic(void) {
    source("ok");
    TEST_ASSERT_FALSE(pe_text_insert(&doc, "\xc0\xaf"));
    TEST_ASSERT_FALSE(pe_text_replace(&doc, 0, 1, "a\0b", 3));
    TEST_ASSERT_EQUAL_STRING("ok", doc.text);
    TEST_ASSERT_EQUAL_INT(0, doc.count);
}

static void test_utf8_boundary(void) {
    source("\xc3\xa4z");
    TEST_ASSERT_FALSE(pe_text_replace(&doc, 1, 2, "x", 1));
    TEST_ASSERT_EQUAL_STRING("\xc3\xa4z", doc.text);
}

static void test_file_limit(void) {
    char *large = malloc(PE_TEXT_MAX + 2);
    TEST_ASSERT_NOT_NULL(large);
    memset(large, 'a', PE_TEXT_MAX + 1);
    large[PE_TEXT_MAX + 1] = 0;
    bool accepted = pe_text_insert(&doc, large);
    large[PE_TEXT_MAX] = 0;
    bool filled = pe_text_insert(&doc, large);
    free(large);
    TEST_ASSERT_FALSE(accepted);
    TEST_ASSERT_TRUE(filled);
    TEST_ASSERT_FALSE(pe_text_insert(&doc, "b"));
    TEST_ASSERT_EQUAL_UINT(PE_TEXT_MAX, doc.length);
}

static void test_undo_redo_cursor(void) {
    source("abc");
    doc.cursor = 3;
    doc.anchor = 1;
    pe_text_insert(&doc, "X");
    TEST_ASSERT_TRUE(pe_text_undo(&doc));
    TEST_ASSERT_EQUAL_STRING("abc", doc.text);
    TEST_ASSERT_EQUAL_UINT(3, doc.cursor);
    TEST_ASSERT_EQUAL_UINT(1, doc.anchor);
    TEST_ASSERT_TRUE(pe_text_redo(&doc));
    TEST_ASSERT_EQUAL_STRING("aX", doc.text);
    TEST_ASSERT_EQUAL_UINT(2, doc.cursor);
}

static void test_undo_new_branch(void) {
    pe_text_insert(&doc, "a");
    pe_text_insert(&doc, "b");
    pe_text_undo(&doc);
    pe_text_insert(&doc, "c");
    TEST_ASSERT_FALSE(pe_text_redo(&doc));
    TEST_ASSERT_EQUAL_STRING("ac", doc.text);
}

static void test_saved_marker(void) {
    pe_text_insert(&doc, "a");
    pe_text_saved(&doc);
    pe_text_insert(&doc, "b");
    TEST_ASSERT_TRUE(pe_text_dirty(&doc));
    pe_text_undo(&doc);
    TEST_ASSERT_FALSE(pe_text_dirty(&doc));
    pe_text_undo(&doc);
    pe_text_insert(&doc, "x");
    TEST_ASSERT_TRUE(pe_text_dirty(&doc));
}

static void test_history_limit(void) {
    for (int i = 0; i < 100; i++) {
        TEST_ASSERT_TRUE(pe_text_insert(&doc, "x"));
    }
    TEST_ASSERT_EQUAL_INT(PE_TEXT_HISTORY, doc.count);
    for (int i = 0; i < PE_TEXT_HISTORY; i++) {
        TEST_ASSERT_TRUE(pe_text_undo(&doc));
    }
    TEST_ASSERT_EQUAL_UINT(36, doc.length);
    TEST_ASSERT_FALSE(pe_text_undo(&doc));
}

static void test_noop_not_dirty(void) {
    source("abc");
    TEST_ASSERT_TRUE(pe_text_replace(&doc, 0, 3, "abc", 3));
    TEST_ASSERT_FALSE(pe_text_dirty(&doc));
    TEST_ASSERT_EQUAL_INT(0, doc.count);
}

static void test_location_tabs_unicode(void) {
    source("a\n\t\xc3\xa4z\n");
    int line, col;
    pe_text_location(&doc, 6, &line, &col);
    TEST_ASSERT_EQUAL_INT(1, line);
    TEST_ASSERT_EQUAL_INT(6, col);
    TEST_ASSERT_EQUAL_UINT(3, pe_text_offset(&doc, 1, 4));
    TEST_ASSERT_EQUAL_UINT(2, pe_text_offset(&doc, 1, 2));
    TEST_ASSERT_EQUAL_UINT(7, pe_text_offset(&doc, 2, 0));
}

static void test_line_bounds(void) {
    source("one\ntwo\n");
    TEST_ASSERT_EQUAL_UINT(4, pe_text_line_start(&doc, 5));
    TEST_ASSERT_EQUAL_UINT(7, pe_text_line_end(&doc, 5));
    TEST_ASSERT_EQUAL_UINT(8, pe_text_offset(&doc, 100, 0));
    TEST_ASSERT_EQUAL_UINT(0, pe_text_offset(&doc, -1, -4));
}

static void test_save_load(void) {
    pe_text_insert(&doc, "export const value = 42;\n");
    TEST_ASSERT_TRUE(pe_text_save(&doc, "test_script.js", error, sizeof error));
    TEST_ASSERT_FALSE(pe_text_dirty(&doc));
    pe_text_insert(&doc, "// changed");
    TEST_ASSERT_TRUE(pe_text_load(&doc, "test_script.js", error, sizeof error));
    TEST_ASSERT_EQUAL_STRING("export const value = 42;\n", doc.text);
    TEST_ASSERT_EQUAL_INT(0, doc.count);
}

static void test_load_normalizes_crlf(void) {
    FILE *f = fopen("test_script.js", "wb");
    TEST_ASSERT_NOT_NULL(f);
    fputs("a\r\nb\rc", f);
    fclose(f);
    TEST_ASSERT_TRUE(pe_text_load(&doc, "test_script.js", error, sizeof error));
    TEST_ASSERT_EQUAL_STRING("a\nb\nc", doc.text);
}

static void test_bad_load_preserves_edits(void) {
    pe_text_insert(&doc, "keep");
    FILE *f = fopen("test_script.js", "wb");
    TEST_ASSERT_NOT_NULL(f);
    fwrite("a\0b", 1, 3, f);
    fclose(f);
    TEST_ASSERT_FALSE(pe_text_load(&doc, "test_script.js", error, sizeof error));
    TEST_ASSERT_EQUAL_STRING("keep", doc.text);
    TEST_ASSERT_EQUAL_INT(1, doc.count);
}

static void test_save_failure(void) {
    pe_text_insert(&doc, "keep");
    TEST_ASSERT_FALSE(pe_text_save(&doc, "missing-directory/script.js", error, sizeof error));
    TEST_ASSERT_TRUE(pe_text_dirty(&doc));
}

static void test_valid_javascript(void) {
    PeJsDiagnostic d =
        pe_js_check("export function update(p) { return {...p, x: 4}; }", "player.js");
    TEST_ASSERT_TRUE_MESSAGE(d.valid, d.message);
}

static void test_invalid_javascript_line(void) {
    PeJsDiagnostic d = pe_js_check("const a = 1;\nconst b = ;\n", "broken.js");
    TEST_ASSERT_FALSE(d.valid);
    TEST_ASSERT_EQUAL_INT(2, d.line);
    TEST_ASSERT_NOT_NULL(strstr(d.message, "SyntaxError"));
}

static void test_compile_only_no_execution(void) {
    PeJsDiagnostic d =
        pe_js_check("throw new Error('must never run'); while (true) {}", "check.js");
    TEST_ASSERT_TRUE_MESSAGE(d.valid, d.message);
}

static void test_undefined_names_not_syntax_errors(void) {
    PeJsDiagnostic d = pe_js_check("missingEngineFunction();", "check.js");
    TEST_ASSERT_TRUE(d.valid);
}

static void test_import_without_execution(void) {
    PeJsDiagnostic d =
        pe_js_check("import {x} from './does-not-exist.js'; export {x};", "check.js");
    TEST_ASSERT_TRUE_MESSAGE(d.valid, d.message);
}

static void test_highlight(void) {
    const char *s = "const a = 42; // note\nMath.floor('x');";
    PeJsToken tokens[128];
    pe_js_highlight(s, tokens, 128);
    TEST_ASSERT_EQUAL_INT(JS_KEYWORD, tokens[0]);
    TEST_ASSERT_EQUAL_INT(JS_NUMBER, tokens[10]);
    TEST_ASSERT_EQUAL_INT(JS_COMMENT, tokens[14]);
    TEST_ASSERT_EQUAL_INT(JS_BUILTIN, tokens[22]);
    TEST_ASSERT_EQUAL_INT(JS_STRING, tokens[33]);
}

static void test_unclosed_highlight(void) {
    PeJsToken tokens[32];
    pe_js_highlight("/* comment", tokens, 32);
    TEST_ASSERT_EQUAL_INT(JS_COMMENT, tokens[9]);
    pe_js_highlight("`template ${value}", tokens, 32);
    TEST_ASSERT_EQUAL_INT(JS_STRING, tokens[15]);
}

static void test_highlight_capacity(void) {
    PeJsToken tokens[] = {JS_PLAIN, JS_STRING};
    pe_js_highlight("const value", tokens, 1);
    TEST_ASSERT_EQUAL_INT(JS_KEYWORD, tokens[0]);
    TEST_ASSERT_EQUAL_INT(JS_STRING, tokens[1]);
}

static void test_keyword_completion(void) {
    pe_text_insert(&doc, "cons");
    PeCompletions c = pe_js_complete(&doc);
    TEST_ASSERT_TRUE(c.count > 0);
    TEST_ASSERT_EQUAL_STRING("const", c.values[0]);
    TEST_ASSERT_TRUE(pe_js_accept(&doc, &c, 0));
    TEST_ASSERT_EQUAL_STRING("const", doc.text);
    pe_text_undo(&doc);
    TEST_ASSERT_EQUAL_STRING("cons", doc.text);
}

static void test_member_completion(void) {
    pe_text_insert(&doc, "Math.fl");
    PeCompletions c = pe_js_complete(&doc);
    TEST_ASSERT_EQUAL_INT(1, c.count);
    TEST_ASSERT_EQUAL_STRING("floor", c.values[0]);
    TEST_ASSERT_TRUE(pe_js_accept(&doc, &c, 0));
    TEST_ASSERT_EQUAL_STRING("Math.floor", doc.text);
}

static void test_document_completion(void) {
    source("const playerSpeed = 90;\nplayerS");
    doc.cursor = doc.anchor = doc.length;
    PeCompletions c = pe_js_complete(&doc);
    TEST_ASSERT_EQUAL_INT(1, c.count);
    TEST_ASSERT_EQUAL_STRING("playerSpeed", c.values[0]);
}

static void test_completion_ignores_strings_comments(void) {
    source("// cons");
    doc.cursor = doc.anchor = doc.length;
    TEST_ASSERT_EQUAL_INT(0, pe_js_complete(&doc).count);
    source("'cons");
    doc.cursor = doc.anchor = doc.length;
    TEST_ASSERT_EQUAL_INT(0, pe_js_complete(&doc).count);
}

static void test_completion_stale_cursor(void) {
    pe_text_insert(&doc, "cons");
    PeCompletions c = pe_js_complete(&doc);
    doc.cursor = doc.anchor = 0;
    TEST_ASSERT_FALSE(pe_js_accept(&doc, &c, 0));
    TEST_ASSERT_EQUAL_STRING("cons", doc.text);
}

static void test_import_does_not_hide_syntax_errors(void) {
    PeJsDiagnostic d = pe_js_check("import {x} from './missing.js'; const value = ;", "check.js");
    TEST_ASSERT_FALSE(d.valid);
    TEST_ASSERT_NOT_NULL(strstr(d.message, "SyntaxError"));
}

static void test_completion_stale_revision(void) {
    pe_text_insert(&doc, "cons");
    PeCompletions c = pe_js_complete(&doc);
    pe_text_replace(&doc, 0, 4, "xxxx", 4);
    TEST_ASSERT_FALSE(pe_js_accept(&doc, &c, 0));
    TEST_ASSERT_EQUAL_STRING("xxxx", doc.text);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_insert_and_selection);
    RUN_TEST(test_utf8_backspace);
    RUN_TEST(test_utf8_delete);
    RUN_TEST(test_invalid_utf8_atomic);
    RUN_TEST(test_utf8_boundary);
    RUN_TEST(test_file_limit);
    RUN_TEST(test_undo_redo_cursor);
    RUN_TEST(test_undo_new_branch);
    RUN_TEST(test_saved_marker);
    RUN_TEST(test_history_limit);
    RUN_TEST(test_noop_not_dirty);
    RUN_TEST(test_location_tabs_unicode);
    RUN_TEST(test_line_bounds);
    RUN_TEST(test_save_load);
    RUN_TEST(test_load_normalizes_crlf);
    RUN_TEST(test_bad_load_preserves_edits);
    RUN_TEST(test_save_failure);
    RUN_TEST(test_valid_javascript);
    RUN_TEST(test_invalid_javascript_line);
    RUN_TEST(test_compile_only_no_execution);
    RUN_TEST(test_undefined_names_not_syntax_errors);
    RUN_TEST(test_import_without_execution);
    RUN_TEST(test_highlight);
    RUN_TEST(test_unclosed_highlight);
    RUN_TEST(test_highlight_capacity);
    RUN_TEST(test_keyword_completion);
    RUN_TEST(test_member_completion);
    RUN_TEST(test_document_completion);
    RUN_TEST(test_completion_ignores_strings_comments);
    RUN_TEST(test_completion_stale_cursor);
    RUN_TEST(test_import_does_not_hide_syntax_errors);
    RUN_TEST(test_completion_stale_revision);
    return UNITY_END();
}
