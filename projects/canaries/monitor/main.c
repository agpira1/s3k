#include "../utils.h"
#include "altc/altio.h"
#include "s3k/s3k.h"
#include "../frame_msg.h"

/* Granted by app0, already loaded into the PMP unit. */
#define MEM_CAP 0
#define UART_CAP 1
#define TIME_CAP 2


#define FRAME_SOCKET_CAP 3

#define MEM_PMP_SLOT 0
#define UART_PMP_SLOT 1

static char trap_stack[1024];

static s3k_msg_t frame_handler(s3k_reply_t request, void *ctx)
{
	(void)ctx;

	uint64_t type = request.data[0];
	uint64_t function = request.data[1];
	uint64_t caller = request.data[2];
	uint64_t sp = request.data[3];

	if (type == FRAME_ENTER) {
		alt_printf(
			"ENTER: function=%X caller=%X sp=%X\n",
			function,
			caller,
			sp);
	} else if (type == FRAME_EXIT) {
		alt_printf(
			"EXIT:  function=%X caller=%X sp=%X\n",
			function,
			caller,
			sp);
	} /*else if (type == FRAME_TOP) {
		alt_printf(
			"preENTER:  frame_top=%X caller=%X sp=%X\n",
			function,
			caller,
			sp);
	}*/

	return (s3k_msg_t){0};
}


int main(void)
{
	util_stack_protect_init();

	util_setup_trap(util_default_trap_handler, trap_stack,
			sizeof trap_stack);

	alt_printf("hello from pid %X at tick %X\n", s3k_get_pid(),
		   s3k_get_time());

	util_dump_caps();

	util_server_loop(FRAME_SOCKET_CAP, frame_handler, NULL, false);
}
