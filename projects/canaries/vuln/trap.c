#include "../interface.h"
#include "altc/altio.h"

/* At VULN_TRAP_PC, installed by the monitor. Returning retries the faulting
 * instruction. */
__attribute__((section(".trap.text"), interrupt("machine"), used,
	       no_instrument_function)) void
trap_handler(void)
{
	s3k_msg_t msg = {
	    .data = {MSG_FAULT, s3k_reg_read(S3K_REG_ECAUSE),
		     s3k_reg_read(S3K_REG_EVAL), s3k_reg_read(S3K_REG_EPC)},
	};
	s3k_reply_t r;

	do {
		r = s3k_sock_sendrecv(VULN_CAP_SOCK, &msg);
	} while (r.err == S3K_ERR_NO_RECEIVER || r.err == S3K_ERR_PREEMPTED);

	// The monitor's resume is the reply.
	if (r.err != S3K_ERR_SUSPENDED)
		alt_printf("trap: sendrecv err %X\n", (uint64_t)r.err);
}
