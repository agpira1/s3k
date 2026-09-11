#include "altc/altio.h"
#include "s3k/s3k.h"

#include <stdint.h>

#include "../../tutorial-commons/utils.h"

#define BUFFER_SIZE 32
#define RET_OFFSET  40 // Use objdump to find the offset on the compiled elf file. 
#define PAYLOAD_SIZE (RET_OFFSET + 8)

static uint8_t payload[PAYLOAD_SIZE];

__attribute__((noinline, noreturn, used)) // used, and noreturn to keep the compiler from removing the code.
void win(void)
{
	alt_puts("Vulnerability exploited!");

	for(;;){
		;
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

	setup_uart();

	// Padding using 40 A's. 
	for (i = 0; i < RET_OFFSET; ++i)
		payload[i] = 'A';

	// Place the address of the win function at the returnaddress. 
	for (i = 0; i < sizeof(target); ++i)
		payload[RET_OFFSET + i] = (uint8_t)(target >> (8 * i));

	vulnerable();

	alt_puts("Vulnerability not exploited!");
}