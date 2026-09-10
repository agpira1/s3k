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

int main(void)
{
	util_setup_trap(util_default_trap_handler, trap_stack,
			sizeof trap_stack);

	alt_printf("hello from pid %X at tick %X\n", s3k_get_pid(),
		   s3k_get_time());

	util_dump_caps();

	while (1)
		;
}
