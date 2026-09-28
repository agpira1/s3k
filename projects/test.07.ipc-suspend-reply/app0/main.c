#include "altc/altio.h"
#include "s3k/s3k.h"

#include "../../tutorial-commons/utils.h"

#define ROUNDS 10

int main(void)
{
	setup_uart();
	alt_puts("app0: start");

	setup_app_1();
	setup_scheduling(ROUND_ROBIN);
	uint32_t sock = setup_socket(true, false, false);
	s3k_reg_write(S3K_REG_SERVTIME, 4500);

	s3k_mon_resume(MONITOR, APP1_PID);

	for (int i = 0; i < ROUNDS; i++) {
		s3k_reply_t r;
		do {
			r = s3k_sock_recv(sock, 0);
		} while (r.err);
		alt_printf("app0: got msg %X\n", (uint64_t)r.data[0]);

		s3k_err_t e = s3k_mon_pmp_unload(MONITOR, APP1_PID,
						 APP_1_CAP_PMP_UART);
		alt_printf("app0: pmp_unload, app1 blocked:   err=%X (INVALID_STATE=%X)\n",
			   (uint64_t)e, (uint64_t)S3K_ERR_INVALID_STATE);

		s3k_mon_suspend(MONITOR, APP1_PID);

		e = s3k_mon_pmp_unload(MONITOR, APP1_PID, APP_1_CAP_PMP_UART);
		alt_printf("app0: pmp_unload, app1 suspended: err=%X\n",
			   (uint64_t)e);
		e = s3k_mon_pmp_load(MONITOR, APP1_PID, APP_1_CAP_PMP_UART,
				     APP_1_PMP_SLOT_UART);
		alt_printf("app0: pmp_load,   app1 suspended: err=%X\n",
			   (uint64_t)e);

		s3k_mon_resume(MONITOR, APP1_PID);
	}

	alt_puts("app0: done");
	for (;;)
		;
}
