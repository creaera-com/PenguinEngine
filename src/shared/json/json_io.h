#ifndef PENGUIN_JSON_IO_H
#define PENGUIN_JSON_IO_H
#include "cJSON.h"
#include <stdbool.h>
#include <stddef.h>

bool pe_json_fail(char *error, size_t size, const char *format, ...);
cJSON *pe_json_read(const char *path, char *error, size_t size);
bool pe_json_integer(const cJSON *json, int minimum, int maximum, int *value);
bool pe_json_relative_path(const char *path, size_t maximum_length);
#endif
