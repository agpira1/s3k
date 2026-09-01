#pragma once

/* Minimal console API used across kernel bring-up and packet handlers. */
void console_init(void);
void console_puts(const char *s);
void console_putc(char c);
void console_puthex(unsigned long value);
