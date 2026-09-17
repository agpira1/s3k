#include "altc/altio.h"
#include "s3k/s3k.h"

#include "../../tutorial-commons/utils.h"

int main(void)
{
	// Setup UART access
	setup_uart();

	// Write hello world.
	alt_printf("Hello, world\n");

	setup_app_1();

	setup_scheduling(ROUND_ROBIN);

	s3k_mon_resume(MONITOR, APP1_PID); // Starts APP1 after giving it memory rights and time slot allocation. 

	while (1)
	{
		alt_printf("Still running!");
		s3k_sleep(s3k_get_time() + S3K_RTC_HZ * 5);
		s3k_cap_revoke(APP_1_TIME);
	}
}
