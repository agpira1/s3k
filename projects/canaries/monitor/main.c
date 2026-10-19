#include "../interface.h"
#include "../utils.h"
#include "altc/altio.h"
#include "s3k/s3k.h"

static char trap_stack[1024];

static void vuln_start(void)
{
	s3k_mon_reg_write(MON_CAP_MONITOR, VULN_PID, S3K_REG_TPC, VULN_TRAP_PC);
	s3k_mon_reg_write(MON_CAP_MONITOR, VULN_PID, S3K_REG_TSP, VULN_TRAP_SP);
	s3k_mon_resume(MON_CAP_MONITOR, VULN_PID);
}

/* Returns true if vuln should be resumed. */
static bool dispatch(const uint64_t *data)
{
	switch (data[0]) {
	case MSG_GUARD_PUSH:
		guard_push(data[1], data[2], data[3]);
		window_invalidate();
		return true;
	case MSG_GUARD_POP:
		guard_pop(data[1]);
		return true;
	case MSG_FAULT:
		return fault_handle(data[1], data[2], data[3]);
	default:
		alt_printf("monitor: unknown message %X\n", data[0]);
		return false;
	}
}

int main(void)
{
	util_stack_protect_init();

	util_setup_trap(util_default_trap_handler, trap_stack,
			sizeof trap_stack);

	alt_printf("hello from pid %X at tick %X\n", s3k_get_pid(),
		   s3k_get_time());

	util_dump_caps();

	vuln_start();

	for (;;) {
		s3k_reply_t r = s3k_sock_recv(MON_CAP_SOCK, 0);

		if (r.err != S3K_SUCCESS)
			continue;

		// vuln's PMP can only change while it is suspended.
		s3k_mon_suspend(MON_CAP_MONITOR, VULN_PID);
		if (dispatch(r.data))
			s3k_mon_resume(MON_CAP_MONITOR, VULN_PID);
	}
}
