#include "penguin/script.h"
#include "quickjs.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static const char *keywords[] = {
    "async",    "await",      "break",   "case",   "catch",    "class", "const",
    "continue", "debugger",   "default", "delete", "do",       "else",  "export",
    "extends",  "false",      "finally", "for",    "function", "if",    "import",
    "in",       "instanceof", "let",     "new",    "null",     "of",    "return",
    "static",   "super",      "switch",  "this",   "throw",    "true",  "try",
    "typeof",   "undefined",  "var",     "void",   "while",    "yield"};
static const char *builtins[] = {"Array",      "Boolean",  "Date",   "Error",   "JSON",
                                 "Map",        "Math",     "Number", "Object",  "Promise",
                                 "RegExp",     "Set",      "String", "Symbol",  "console",
                                 "parseFloat", "parseInt", "isNaN",  "isFinite"};
static bool identifier(unsigned char c) {
    return isalnum(c) || c == '_' || c == '$' || c >= 128;
}
static bool matches(const char *s, size_t n, const char *const *list, size_t count) {
    for (size_t i = 0; i < count; i++)
        if (strlen(list[i]) == n && !memcmp(s, list[i], n))
            return true;
    return false;
}
/* QuickJS resolves imports after parsing. Abort that phase deliberately:
   a reached loader proves source parsing succeeded without loading project files. */
static JSModuleDef *skip_module_resolution(JSContext *context, const char *name, void *opaque) {
    (void)name;
    *(bool *)opaque = true;
    JS_ThrowReferenceError(context, "Module resolution skipped during syntax checking");
    return NULL;
}
PeJsDiagnostic pe_js_check(const char *source, const char *filename) {
    PeJsDiagnostic d = {false, 0, 0, "JavaScript parser unavailable"};
    if (strlen(source) > PE_TEXT_MAX) {
        snprintf(d.message, sizeof d.message, "Script exceeds 64 KiB");
        return d;
    }
    JSRuntime *runtime = JS_NewRuntime();
    if (!runtime)
        return d;
    bool resolution_skipped = false;
    JS_SetModuleLoaderFunc(runtime, NULL, skip_module_resolution, &resolution_skipped);
    JS_SetMemoryLimit(runtime, 64 * 1024 * 1024);
    JS_SetMaxStackSize(runtime, 256 * 1024);
    JSContext *context = JS_NewContext(runtime);
    if (!context) {
        JS_FreeRuntime(runtime);
        return d;
    }
    JSValue compiled = JS_Eval(context, source, strlen(source), filename ? filename : "untitled.js",
                               JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);
    if (!JS_IsException(compiled)) {
        d.valid = true;
        snprintf(d.message, sizeof d.message, "JavaScript syntax OK");
    } else {
        JSValue exception = JS_GetException(context);
        const char *message = JS_ToCString(context, exception);
        snprintf(d.message, sizeof d.message, "%s", message ? message : "JavaScript syntax error");
        JS_FreeCString(context, message);
        JSValue line = JS_GetPropertyStr(context, exception, "lineNumber"),
                column = JS_GetPropertyStr(context, exception, "columnNumber");
        int32_t value = 0;
        if (!JS_ToInt32(context, &value, line) && value > 0)
            d.line = value;
        if (!JS_ToInt32(context, &value, column) && value > 0)
            d.column = value;
        JS_FreeValue(context, line);
        JS_FreeValue(context, column);
        JSValue stack = JS_GetPropertyStr(context, exception, "stack");
        const char *stack_text = JS_ToCString(context, stack);
        if (!d.line && stack_text) { /* Parser stacks include filename:line:column. */
            const char *name = filename ? filename : "untitled.js";
            const char *p = strstr(stack_text, name);
            if (p) {
                p += strlen(name);
                if (*p == ':') {
                    char *end;
                    d.line = (int)strtol(p + 1, &end, 10);
                    p = end;
                    if (*p == ':')
                        d.column = (int)strtol(p + 1, NULL, 10);
                }
            }
        }
        JS_FreeCString(context, stack_text);
        JS_FreeValue(context, stack);
        JS_FreeValue(context, exception);
        if (resolution_skipped) {
            d.valid = true;
            d.line = d.column = 0;
            snprintf(d.message, sizeof d.message, "JavaScript syntax OK (imports not resolved)");
        }
    }
    JS_FreeValue(context, compiled);
    JS_FreeContext(context);
    JS_FreeRuntime(runtime);
    return d;
}
void pe_js_highlight(const char *s, PeJsToken *tokens, size_t capacity) {
    size_t n = strlen(s);
    size_t i = 0;
    while (i < n) {
        size_t start = i;
        PeJsToken kind = JS_PLAIN;
        unsigned char c = (unsigned char)s[i++];
        if (c == '/' && i < n && s[i] == '/') {
            kind = JS_COMMENT;
            while (i < n && s[i] != '\n')
                i++;
        } else if (c == '/' && i < n && s[i] == '*') {
            kind = JS_COMMENT;
            i++;
            while (i < n) {
                if (s[i] == '*' && i + 1 < n && s[i + 1] == '/') {
                    i += 2;
                    break;
                }
                i++;
            }
        } else if (c == '\'' || c == '"' || c == '`') {
            kind = JS_STRING;
            while (i < n) {
                char next = s[i++];
                if (next == '\\' && i < n)
                    i++;
                else if (next == c)
                    break;
            }
        } else if (isdigit(c)) {
            kind = JS_NUMBER;
            while (i < n && (isalnum((unsigned char)s[i]) || s[i] == '.' || s[i] == '_'))
                i++;
        } else if (identifier(c)) {
            while (i < n && identifier((unsigned char)s[i]))
                i++;
            if (matches(s + start, i - start, keywords, sizeof keywords / sizeof *keywords))
                kind = JS_KEYWORD;
            else if (matches(s + start, i - start, builtins, sizeof builtins / sizeof *builtins))
                kind = JS_BUILTIN;
        }
        for (size_t k = start; k < i && k < capacity; k++)
            tokens[k] = kind;
    }
}
static void suggestion(PeCompletions *result, const char *s, size_t n, const char *prefix,
                       size_t length) {
    if (n <= length || n >= PE_COMPLETION_MAX || memcmp(s, prefix, length) ||
        result->count == PE_COMPLETIONS)
        return;
    for (int i = 0; i < result->count; i++)
        if (strlen(result->values[i]) == n && !memcmp(result->values[i], s, n))
            return;
    memcpy(result->values[result->count], s, n);
    result->values[result->count++][n] = 0;
}
PeCompletions pe_js_complete(const PeText *d) {
    PeCompletions result = {0};
    result.revision = d->revision;
    result.start = result.end = d->cursor;
    if (d->cursor != d->anchor)
        return result;
    while (result.start && identifier((unsigned char)d->text[result.start - 1]))
        result.start--;
    size_t length = result.end - result.start;
    if (length >= PE_COMPLETION_MAX)
        return result;
    PeJsToken *tokens = malloc((d->length + 1) * sizeof *tokens);
    if (!tokens)
        return result;
    pe_js_highlight(d->text, tokens, d->length);
    if (result.start && ((result.start < d->length && (tokens[result.start] == JS_STRING ||
                                                       tokens[result.start] == JS_COMMENT)) ||
                         (result.start == d->length && (tokens[result.start - 1] == JS_COMMENT)))) {
        free(tokens);
        return result;
    }
    const char *prefix = d->text + result.start;
    bool dot = result.start && d->text[result.start - 1] == '.';
    if (dot) {
        size_t begin = result.start - 1;
        while (begin && identifier((unsigned char)d->text[begin - 1]))
            begin--;
        size_t owner = result.start - 1 - begin;
        const char *const *list = NULL;
        size_t count = 0;
        static const char *math[] = {"abs",   "ceil", "floor", "max", "min", "random",
                                     "round", "sqrt", "sin",   "cos", "PI"},
                          *json[] = {"parse", "stringify"},
                          *console[] = {"log", "warn", "error", "info", "debug"};
        if (owner == 4 && !memcmp(d->text + begin, "Math", 4)) {
            list = math;
            count = sizeof math / sizeof *math;
        } else if (owner == 4 && !memcmp(d->text + begin, "JSON", 4)) {
            list = json;
            count = sizeof json / sizeof *json;
        } else if (owner == 7 && !memcmp(d->text + begin, "console", 7)) {
            list = console;
            count = sizeof console / sizeof *console;
        }
        for (size_t i = 0; i < count; i++)
            suggestion(&result, list[i], strlen(list[i]), prefix, length);
    } else {
        for (size_t i = 0; i < sizeof keywords / sizeof *keywords; i++)
            suggestion(&result, keywords[i], strlen(keywords[i]), prefix, length);
        for (size_t i = 0; i < sizeof builtins / sizeof *builtins; i++)
            suggestion(&result, builtins[i], strlen(builtins[i]), prefix, length);
        for (size_t i = 0; i < d->length;) {
            if (tokens[i] == JS_PLAIN && identifier((unsigned char)d->text[i]) &&
                !isdigit((unsigned char)d->text[i])) {
                size_t start = i++;
                while (i < d->length && identifier((unsigned char)d->text[i]))
                    i++;
                suggestion(&result, d->text + start, i - start, prefix, length);
            } else
                i++;
        }
    }
    free(tokens);
    return result;
}
bool pe_js_accept(PeText *d, const PeCompletions *c, int index) {
    if (index < 0 || index >= c->count || d->revision != c->revision || d->cursor != c->end ||
        d->anchor != d->cursor)
        return false;
    return pe_text_replace(d, c->start, c->end, c->values[index], strlen(c->values[index]));
}
