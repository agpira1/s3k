#include "altc/altio.h"
#include "s3k/s3k.h"

#include "../../tutorial-commons/utils.h"

__attribute__((used, externally_visible))
uintptr_t __stack_chk_guard = (uintptr_t)0x0000111122223333ULL;

__attribute__((noreturn, no_stack_protector, used, externally_visible))
void __stack_chk_fail(void) {

	alt_printf("Stack canary hit!\n");

	s3k_cap_revoke(APP_1_TIME);
	
	while (1)
	{
		;
	}
}

/*
__attribute__((noinline))
int canary_test(void)
{
    volatile char buffer[32];
    buffer[0] = 42;
    return buffer[0];
}
*/

int main(void)
{
	// Setup UART access
	setup_uart();

	setup_app_1();

	setup_scheduling(ROUND_ROBIN);

	uint32_t socket = setup_socket(true, false, false);

	//canary_test();
	s3k_mon_resume(MONITOR, APP1_PID); // Starts APP1 after giving it memory rights and time slot allocation. 

	while (1)
	{
		s3k_reply_t message = s3k_sock_recv(socket, 0);

		if (message.data[0] == 1)
		{
			alt_puts("Stack canary hit!");
			s3k_cap_revoke(APP_1_TIME);
			s3k_sync();
		}
	}
}
