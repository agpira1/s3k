#include "altc/altio.h"
#include "s3k/s3k.h"

#include "../../tutorial-commons/utils.h"

#define BUFFER_SIZE 32
#define RET_OFFSET  40 // Use objdump to find the offset on the compiled elf file. 
#define PAYLOAD_SIZE (RET_OFFSET + 8)

static uint8_t payload[PAYLOAD_SIZE];

__attribute__((used, externally_visible))
uintptr_t __stack_chk_guard = (uintptr_t)0x0000111122223333ULL;

__attribute__((noreturn, no_stack_protector, used, externally_visible))
void __stack_chk_fail(void) {

	alt_printf("Stack canary hit! from app1\n");

	s3k_msg_t message = {0};
	message.data[0] = 1;

	while (s3k_sock_send(APP_1_CAP_SOCKET, &message) != 0)
	{
		s3k_sleep(s3k_get_time() + S3K_RTC_HZ);
	}
	
	while (1)
	{
		;
	}
}

__attribute__((noinline, noreturn, used)) // used, and noreturn to keep the compiler from removing the code.
void win(void)
{
	alt_puts("Vulnerability exploited!");

	while (1)
	{
		alt_printf("Still running! from app1");
		s3k_sleep(s3k_get_time() + S3K_RTC_HZ);
	}
}

__attribute__((noinline))
static void unchecked_copy(volatile uint8_t *destination, const uint8_t *source) // Unsafe by design. The function does not check if the destination is large enough. 
{
	unsigned int i;

	for (i = 0; i < PAYLOAD_SIZE; ++i)
		destination[i] = source[i];
}

__attribute__((noinline))
static void vulnerable(void)
{
	uint8_t buffer[BUFFER_SIZE];

	unchecked_copy(buffer, payload); // Writes 48 bytes into 32 bytes. Offset to ra should be 40. Otherwise change. 

}

int main(void)
{
	uintptr_t target = (uintptr_t)win;
	int i;

	// Padding using 40 A's. 
	for (i = 0; i < RET_OFFSET; ++i)
		payload[i] = 'A';

	// Place the address of the win function at the returnaddress. 
	for (i = 0; i < sizeof(target); ++i)
		payload[RET_OFFSET + i] = (uint8_t)(target >> (8 * i));

	vulnerable();

	alt_puts("Vulnerability not exploited!");
}