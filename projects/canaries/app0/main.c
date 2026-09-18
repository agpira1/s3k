#include "../utils.h"
#include "altc/altio.h"
#include "s3k/s3k.h"

#define UART_CAP FREE_CAP_BGN
#define UART_PMP_SLOT 1

/* monitor */
#define MON_APP_PID 1
#define MON_APP_BASE 0x80020000
#define MON_APP_SIZE 0x10000

/* Slots in monitor's capability table, and monitor's hardware PMP slots. */
#define MON_APP_MEM_CAP 0
#define MON_APP_UART_CAP 1
#define MON_APP_TIME_CAP 2
#define MON_APP_MEM_SLOT 0
#define MON_APP_UART_SLOT 1

/* Time slots on hart 0, one even share per application. */
#define MON_APP_TIME_BGN 0
#define MON_APP_TIME_END 10

/* vuln */
#define VULN_PID 2
#define VULN_BASE 0x80030000
#define VULN_SIZE 0x10000

/* Slots in vuln's capability table, and vuln's hardware PMP slots. */
#define VULN_MEM_CAP 0
#define VULN_UART_CAP 1
#define VULN_TIME_CAP 2
#define VULN_MEM_SLOT 0
#define VULN_UART_SLOT 1

/* Time slots on hart 0, one even share per application. */
#define VULN_TIME_BGN 10
#define VULN_TIME_END 20

static char trap_stack[1024];

int main(void)
{
	util_stack_protect_init();

	// Set up I/O and trap handler for boot process.
	s3k_err_t err;

	err = util_setup_uart(UART_MEM, UART_CAP, UART_PMP_SLOT,
			      UART0_BASE_ADDR, 0x8);
	if (err != S3K_SUCCESS)
		return 1;

	util_setup_trap(util_default_trap_handler, trap_stack,
			sizeof trap_stack);

	alt_printf("hello from pid %X at tick %X\n", s3k_get_pid(),
		   s3k_get_time());

	// Set up and resume monitor
	s3k_cidx_t monitor_mem;

	err = util_grant_memory(MON_APP_PID, RAM_MEM, MON_APP_BASE,
				MON_APP_SIZE, S3K_MEM_RWX, MON_APP_MEM_CAP,
				MON_APP_MEM_SLOT, &monitor_mem);
	util_check("monitor memory", err);

	err = util_grant_device(MON_APP_PID, UART_MEM, UART0_BASE_ADDR, 0x8,
				S3K_MEM_RW, MON_APP_UART_CAP,
				MON_APP_UART_SLOT);
	util_check("monitor uart", err);

	err = util_grant_time(MON_APP_PID, HART0_TIME, MON_APP_TIME_BGN,
			      MON_APP_TIME_END, MON_APP_TIME_CAP);
	util_check("monitor time", err);
	util_check("monitor start", util_start(MON_APP_PID, MON_APP_BASE));

	// Set up and resume vuln
	s3k_cidx_t vuln_mem;

	err = util_grant_memory(VULN_PID, RAM_MEM, VULN_BASE, VULN_SIZE,
				S3K_MEM_RWX, VULN_MEM_CAP, VULN_MEM_SLOT,
				&vuln_mem);
	util_check("vuln memory", err);

	err = util_grant_device(VULN_PID, UART_MEM, UART0_BASE_ADDR, 0x8,
				S3K_MEM_RW, VULN_UART_CAP, VULN_UART_SLOT);
	util_check("vuln uart", err);

	err = util_grant_time(VULN_PID, HART0_TIME, VULN_TIME_BGN,
			      VULN_TIME_END, VULN_TIME_CAP);
	util_check("vuln time", err);
	util_check("vuln start", util_start(VULN_PID, VULN_BASE));

	util_dump_caps();

	while (1)
		;
}
