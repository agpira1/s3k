#include "altc/altio.h"
#include "s3k/s3k.h"

#include "../../tutorial-commons/utils.h"

#define ROUNDS 10

int main(void)
{
	alt_puts("app1: start");

	for (int i = 1; i <= ROUNDS; i++) {
		s3k_msg_t msg = { .data = { i } };
		s3k_reply_t r;
		do {
			r = s3k_sock_sendrecv(APP_1_CAP_SOCKET, &msg);
		} while (r.err == S3K_ERR_NO_RECEIVER
			 || r.err == S3K_ERR_PREEMPTED);
		alt_printf("app1: sendrecv %X returned err=%X (SUSPENDED=%X)\n",
			   (uint64_t)i, (uint64_t)r.err,
			   (uint64_t)S3K_ERR_SUSPENDED);
	}

	alt_puts("app1: done");
	for (;;)
		;
}
