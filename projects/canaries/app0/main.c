#include "../interface.h"
#include "../utils.h"
#include "altc/altio.h"
#include "s3k/s3k.h"

#define UART_CAP FREE_CAP_BGN
#define UART_PMP_SLOT 1

/* monitor */
#define MON_BASE 0x80020000
#define MON_SIZE 0x10000
#define MON_SLOT_MEM 0
#define MON_SLOT_UART 1

/* Time slots on hart 0, one even share per application. */
#define MON_TIME_BGN 0
#define MON_TIME_END 10
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

	// Memory and time are carved in increasing order: monitor first.
	util_grant_memory(MONITOR_PID, RAM_MEM, MON_BASE, MON_SIZE, S3K_MEM_RWX,
			  MON_CAP_MEM, MON_SLOT_MEM, NULL);
	util_grant_device(MONITOR_PID, UART_MEM, UART0_BASE_ADDR, 0x8,
			  S3K_MEM_RW, MON_CAP_UART, MON_SLOT_UART);
	util_grant_time(MONITOR_PID, HART0_TIME, MON_TIME_BGN, MON_TIME_END,
			MON_CAP_TIME);

	// vuln is not resumed here, only initialized.
	util_grant_memory(VULN_PID, RAM_MEM, VULN_IMAGE_BASE, VULN_IMAGE_SIZE,
			  S3K_MEM_RWX, VULN_CAP_IMAGE, VULN_SLOT_IMAGE, NULL);
	util_grant_device(VULN_PID, UART_MEM, UART0_BASE_ADDR, 0x8, S3K_MEM_RW,
			  VULN_CAP_UART, VULN_SLOT_UART);
	util_grant_time(VULN_PID, HART0_TIME, VULN_TIME_BGN, VULN_TIME_END,
			VULN_CAP_TIME);

	s3k_mon_reg_write(MONITOR, VULN_PID, S3K_REG_PC, VULN_IMAGE_BASE);

	s3k_cidx_t srv, cli;

	util_make_socket_pair(GUARD_CHAN, S3K_IPC_NOYIELD, S3K_IPC_CDATA,
			      VULN_PID, &srv, &cli);
	s3k_mon_cap_move(MONITOR, s3k_get_pid(), cli, VULN_PID, VULN_CAP_SOCK);
	s3k_mon_cap_move(MONITOR, s3k_get_pid(), srv, MONITOR_PID,
			 MON_CAP_SOCK);

	s3k_cidx_t stack = util_find_free_cap();

	s3k_cap_derive(RAM_MEM, stack,
		       s3k_mk_memory(VULN_STACK_BASE, VULN_STACK_TOP,
				     S3K_MEM_RW));
	s3k_mon_cap_move(MONITOR, s3k_get_pid(), stack, MONITOR_PID,
			 MON_CAP_STACK_MEM);

	// Deriving from MONITOR raises its mark past the child, disabling it
	// for lower pids, so we make one for the monitor too.
	s3k_cidx_t mon_mon = util_find_free_cap();

	s3k_cap_derive(MONITOR, mon_mon,
		       s3k_mk_monitor(MONITOR_PID, MONITOR_PID + 1));

	s3k_cidx_t vuln_mon = util_find_free_cap();

	s3k_cap_derive(MONITOR, vuln_mon,
		       s3k_mk_monitor(VULN_PID, VULN_PID + 1));
	s3k_mon_cap_move(mon_mon, s3k_get_pid(), vuln_mon, MONITOR_PID,
			 MON_CAP_MONITOR);

	s3k_mon_reg_write(mon_mon, MONITOR_PID, S3K_REG_PC, MON_BASE);
	s3k_mon_resume(mon_mon, MONITOR_PID);

	util_dump_caps();

	while (1)
		;
}
