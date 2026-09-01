#include "int_service.h"

#include <limits.h>

/* Strict signed integer parser shared by HTTP and telnet. */
int int_service_parse(const char *s, long long *value) {
    while (*s == ' ' || *s == '\t') {
        ++s;
    }

    int negative = 0;
    if (*s == '+' || *s == '-') {
        negative = (*s == '-');
        ++s;
    }

    if (*s < '0' || *s > '9') {
        return -1;
    }

    unsigned long long acc = 0;
    while (*s >= '0' && *s <= '9') {
        unsigned digit = (unsigned)(*s - '0');
        if (acc > (ULLONG_MAX - digit) / 10ULL) {
            return -1;
        }
        acc = acc * 10ULL + digit;
        ++s;
    }

    while (*s == ' ' || *s == '\t' || *s == '\r') {
        ++s;
    }

    if (*s != '\0') {
        return -1;
    }

    if (negative) {
        if (acc > (unsigned long long)LLONG_MAX + 1ULL) {
            return -1;
        }
        *value = acc == (unsigned long long)LLONG_MAX + 1ULL ? LLONG_MIN : -(long long)acc;
    } else {
        if (acc > (unsigned long long)LLONG_MAX) {
            return -1;
        }
        *value = (long long)acc;
    }

    return 0;
}

size_t int_service_format(long long value, char *out, size_t out_len) {
    /* Format without libc so the kernel stays freestanding. */
    char tmp[32];
    size_t pos = 0;
    unsigned long long magnitude;

    if (out_len == 0) {
        return 0;
    }

    magnitude = value < 0 ? (unsigned long long)(-(value + 1)) + 1ULL : (unsigned long long)value;

    if (magnitude == 0) {
        tmp[pos++] = '0';
    } else {
        while (magnitude && pos < sizeof(tmp)) {
            tmp[pos++] = (char)('0' + (magnitude % 10ULL));
            magnitude /= 10ULL;
        }
    }

    if (value < 0 && pos < sizeof(tmp)) {
        tmp[pos++] = '-';
    }

    size_t written = 0;
    while (pos && written + 1 < out_len) {
        out[written++] = tmp[--pos];
    }
    out[written] = '\0';
    return written;
}
