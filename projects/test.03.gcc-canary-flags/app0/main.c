#include "altc/altio.h"
#include "s3k/s3k.h"

#include "../../tutorial-commons/utils.h"

__attribute__((used, externally_visible))
uintptr_t __stack_chk_guard = (uintptr_t)0x0000111122223333ULL;

__attribute__((noreturn, no_stack_protector, used, externally_visible))
void __stack_chk_fail(void) {

	alt_printf("Stack canary hit!\n");

	for(;;){
		;
	}
}

__attribute__((noinline))
int canary_test(void)
{
    volatile char buffer[32];
    buffer[0] = 42;
    return buffer[0];
}

int main(void)
{
	// Setup UART access
	setup_uart();
	
	canary_test();

	// Write hello world.
	alt_printf("Hello, world\n");
}
