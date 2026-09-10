#include "altc/altio.h"
#include "drivers/uart.h"
#include "plat/config.h"

#include <stdarg.h>

int alt_getchar(void)
{
	return uart_getc((void *)UART0_BASE_ADDR);
}

int alt_gets(char *src)
{
	int i = 0;
	while (1) {
		src[i] = alt_getchar();
		if (src[i] == '\n' || src[i] == '\r' || src[i] == '\0')
			break;
		/* Backspace or DEL: rub out the previous character. */
		if (src[i] == '\b' || src[i] == 0x7f) {
			if (i > 0) {
				i--;
				alt_putstr("\b \b");
			}
			continue;
		}
		alt_putchar(src[i]);
		i++;
	}
	alt_putchar('\n');
	src[i] = '\0';
	return i;
}
