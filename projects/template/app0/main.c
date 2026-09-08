#include "altc/altio.h"
#include "s3k/s3k.h"
#include "utils.h"

#define UART_CAP FREE_CAP_BGN
#define UART_PMP_SLOT 1

static char trap_stack[1024];

int main(void)
{
	s3k_err_t err;

	err = util_setup_uart(UART_MEM, UART_CAP, UART_PMP_SLOT,
			      UART0_BASE_ADDR, 0x8);
	if (err != S3K_SUCCESS)
		return 1;

	util_setup_trap(util_default_trap_handler, trap_stack,
			sizeof trap_stack);

	alt_printf("hello from pid %X at tick %X\n", s3k_get_pid(),
		   s3k_get_time());

	util_dump_caps();

	while (1)
		;
}
