#include "shared/json/json_io.h"
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool pe_json_fail(char *error, size_t size, const char *fmt, ...) {
    if (error && size) {
        va_list args;
        va_start(args, fmt);
        vsnprintf(error, size, fmt, args);
        va_end(args);
    }
    return false;
}

cJSON *pe_json_read(const char *path, char *error, size_t size) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        pe_json_fail(error, size, "Cannot open %s", path);
        return NULL;
    }
    if (fseek(f, 0, SEEK_END)) {
        fclose(f);
        pe_json_fail(error, size, "Cannot seek file: %s", path);
        return NULL;
    }
    long n = ftell(f);
    if (n < 0 || n > 4 * 1024 * 1024) {
        fclose(f);
        pe_json_fail(error, size, "File too large or unreadable");
        return NULL;
    }
    rewind(f);
    char *text = malloc((size_t)n + 1);
    if (!text) {
        fclose(f);
        pe_json_fail(error, size, "Out of memory");
        return NULL;
    }
    bool ok = fread(text, 1, (size_t)n, f) == (size_t)n;
    fclose(f);
    text[n] = 0;
    ok = ok && !memchr(text, 0, (size_t)n);
    cJSON *json = ok ? cJSON_ParseWithLengthOpts(text, (size_t)n + 1, NULL, true) : NULL;
    free(text);
    if (!json)
        pe_json_fail(error, size, "Invalid JSON: %s", path);
    return json;
}

bool pe_json_integer(const cJSON *j, int min, int max, int *value) {
    if (!cJSON_IsNumber(j) || !isfinite(j->valuedouble) || j->valuedouble < min ||
        j->valuedouble > max || floor(j->valuedouble) != j->valuedouble)
        return false;
    *value = (int)j->valuedouble;
    return true;
}

bool pe_json_relative_path(const char *p, size_t maximum_length) {
    if (!p || !*p || strlen(p) >= maximum_length || *p == '/' || *p == '\\' || strchr(p, ':') ||
        strchr(p, '\\'))
        return false;
    const char *part = p;
    for (const char *c = p;; c++) {
        if (*c == '/' || !*c) {
            size_t n = (size_t)(c - part);
            if (!n || (n == 2 && part[0] == '.' && part[1] == '.') || (n == 1 && part[0] == '.'))
                return false;
            part = c + 1;
            if (!*c)
                break;
        }
    }
    return true;
}
