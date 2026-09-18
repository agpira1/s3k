#include "../utils.h"
#include "altc/altio.h"
#include "s3k/s3k.h"

/* Granted by app0, already loaded into the PMP unit. */
#define MEM_CAP 0
#define UART_CAP 1
#define TIME_CAP 2

#define MEM_PMP_SLOT 0
#define UART_PMP_SLOT 1

static char trap_stack[1024];

__attribute__((noinline, used)) void win(void)
{
	alt_puts("You exploited the binary!");
	while (1)
		;
}

__attribute__((noinline)) static void greet(void)
{
	char name[64];

	alt_puts("What is your name?");
	alt_gets(name);
	alt_printf("Hello, %s!\n", name);
}

int main(void)
{
	util_stack_protect_init();

	util_setup_trap(util_default_trap_handler, trap_stack,
			sizeof trap_stack);

	alt_printf("hello from pid %X at tick %X\n", s3k_get_pid(),
		   s3k_get_time());

	util_dump_caps();

	greet();

	while (1)
		;
}
