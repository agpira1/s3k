#include "altc/altio.h"
#include "s3k/s3k.h"

#include "../../tutorial-commons/utils.h"

int main(void)
{
	// Write hello world.
	alt_puts("hello, world from app1");
	alt_printf("Hello, world\n");

	alt_printf("I have execution time!\n");

	alt_puts("hello, world from app1 test test");

	alt_printf("I don't have execution time!\n");

		while (1)
	{
		alt_printf("Still running! from app1");
		s3k_sleep(s3k_get_time() + S3K_RTC_HZ);
		
	}
}
