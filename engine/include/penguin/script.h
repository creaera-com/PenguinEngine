#ifndef PENGUIN_SCRIPT_H
#define PENGUIN_SCRIPT_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define PE_TEXT_MAX 65536
#define PE_TEXT_HISTORY 64
#define PE_COMPLETIONS 12
#define PE_COMPLETION_MAX 64

typedef struct {
    char *before, *after;
    size_t before_cursor, before_anchor, after_cursor;
    uint64_t before_revision, after_revision;
} PeTextEdit;
typedef struct {
    char text[PE_TEXT_MAX + 1];
    size_t length, cursor, anchor;
    PeTextEdit edits[PE_TEXT_HISTORY];
    int count, position;
    uint64_t revision, next_revision, saved_revision;
} PeText;
typedef enum { JS_PLAIN, JS_KEYWORD, JS_STRING, JS_COMMENT, JS_NUMBER, JS_BUILTIN } PeJsToken;
typedef struct {
    bool valid;
    int line, column;
    char message[256];
} PeJsDiagnostic;
typedef struct {
    char values[PE_COMPLETIONS][PE_COMPLETION_MAX];
    int count;
    size_t start, end;
    uint64_t revision;
} PeCompletions;
/* Initialize once; free before reinitializing. Text must be UTF-8 without embedded NUL. */
bool pe_text_init(PeText *document, const char *source);
void pe_text_free(PeText *document);
bool pe_text_replace(PeText *document, size_t start, size_t end, const char *bytes, size_t length);
bool pe_text_insert(PeText *document, const char *text);
bool pe_text_backspace(PeText *document);
bool pe_text_delete(PeText *document);
bool pe_text_undo(PeText *document);
bool pe_text_redo(PeText *document);
bool pe_text_dirty(const PeText *document);
void pe_text_saved(PeText *document);
size_t pe_text_previous(const PeText *document, size_t offset);
size_t pe_text_next(const PeText *document, size_t offset);
size_t pe_text_line_start(const PeText *document, size_t offset);
size_t pe_text_line_end(const PeText *document, size_t offset);
void pe_text_location(const PeText *document, size_t offset, int *line, int *column);
size_t pe_text_offset(const PeText *document, int line, int column);
bool pe_text_load(PeText *document, const char *path, char *error, size_t size);
bool pe_text_save(PeText *document, const char *path, char *error, size_t size);
/* Real parser check: compile only. Never executes scripts or resolves imported modules. */
PeJsDiagnostic pe_js_check(const char *source, const char *filename);
/* One token kind per source byte. Template strings are highlighted as a single string. */
void pe_js_highlight(const char *source, PeJsToken *tokens, size_t capacity);
PeCompletions pe_js_complete(const PeText *document);
bool pe_js_accept(PeText *document, const PeCompletions *completions, int index);
#endif
