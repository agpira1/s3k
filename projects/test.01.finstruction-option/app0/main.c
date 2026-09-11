#include "altc/altio.h"
#include "s3k/s3k.h"
#include <stdint.h>

#include "../../tutorial-commons/utils.h"

static volatile uint64_t enter_count;
static volatile uint64_t exit_count;

__attribute__((no_instrument_function))
void __cyg_profile_func_enter(void *function, void *caller) 
{
	(void)function;
	(void)caller;
	
    enter_count++;
}

__attribute__((no_instrument_function))
void __cyg_profile_func_exit(void *function, void *caller)
{
	(void)function;
	(void)caller;

    exit_count++;
}

int main(void)
{
	// Setup UART access
	setup_uart();

	// Write hello world.
	alt_printf("Hello, world\n");
	alt_printf("Enter count: %x\n", enter_count); // Prints 2 because of setup_uart.
	alt_printf("Exit count: %x", exit_count); // Prints 1 because main hasn't exited yet.
}
