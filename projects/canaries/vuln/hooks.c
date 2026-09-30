#include "../frame_msg.h"

#include "s3k/s3k.h"
#include <stdbool.h>
#include <stdint.h>
#include "altc/altio.h"

#define MONITOR_SOCK_CAP 3

static bool hooks_ready = false;
/*
static inline __attribute__((no_instrument_function)) uintptr_t read_sp(void) // Does this read the correct pointer?
{
	uintptr_t sp;

	__asm__ volatile("mv %0, sp" : "=r"(sp)); //Write the assembly inline at place of the function. Saves sp to a variable. 

	return sp;
}
*/
static __attribute__((no_instrument_function))
void send_frame_event(uint64_t type, void *function, void *caller, uintptr_t sp)
{
	s3k_msg_t msg = {0};

	//Do not send anything until main() has installed the socket.
	
	if (!hooks_ready)
		return;

	msg.data[0] = type; //FRAME_ENTER or FRAME_EXIT
	msg.data[1] = (uintptr_t)function;
	msg.data[2] = (uintptr_t)caller;
	msg.data[3] = sp;

	(void)s3k_sock_sendrecv(MONITOR_SOCK_CAP, &msg);
}


void __attribute__((no_instrument_function))
__cyg_profile_func_enter_c(void *function, void *caller, uintptr_t caller_sp) //Original function placed in hooks.S, this is a callee inside of that function.
{	
	//uintptr_t sp = read_sp();
	//alt_printf("SP TAKEN INSIDE FINSTRUCTION, %X\n", sp);
	//void* frame_address = __builtin_frame_address(0); //1 for one layer earlier which should capture the correct frame top. 
	//send_frame_event(FRAME_TOP, frame_address, 0, 0);
	send_frame_event(FRAME_ENTER, function, caller, caller_sp);
}

void __attribute__((no_instrument_function))
__cyg_profile_func_exit(void *function, void *caller)
{	
	//uintptr_t sp = read_sp();
	send_frame_event(FRAME_EXIT, function, caller, 0);
}

__attribute__((no_instrument_function))
void hooks_enable(void)
{
	hooks_ready = true;
}