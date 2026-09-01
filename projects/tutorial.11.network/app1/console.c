#include "console.h"
#include <stdio.h>

void console_puts(const char *s) {
    printf(s);
}

void console_putc(char c) {
    printf("%c", c);
}

void console_puthex(unsigned long value) {
    printf("%lx", value);
}
