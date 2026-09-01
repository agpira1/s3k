#pragma once

#include <stddef.h>

/* Shared integer parse/format helpers for the application protocols. */
int int_service_parse(const char *s, long long *value);
size_t int_service_format(long long value, char *out, size_t out_len);
