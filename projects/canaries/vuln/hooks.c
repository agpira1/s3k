#include "../frame_msg.h"

#include "s3k/s3k.h"
#include <stdbool.h>
#include <stdint.h>
#include "altc/altio.h"
#include "../interface.h"
#include "stack_usage.h"

#define MONITOR_SOCK_CAP 3

static bool hooks_ready = false;

static __attribute__((no_instrument_function))
void send_frame_event(uint64_t type, uintptr_t canary, uintptr_t stack_frame_high, void* function)
{
	s3k_msg_t msg = {0};

	//Do not send anything until main() has installed the socket.
	
	if (!hooks_ready) {
		return;
	}

	msg.data[0] = type; //MSG_GUARD_PUSH or MSG_GUARD_POP
	msg.data[1] = canary;
	msg.data[2] = stack_frame_high; // Caller is not necessary. So we swap this to canary offset. 
	msg.data[3] = (uintptr_t)function;

	(void)s3k_sock_sendrecv(MONITOR_SOCK_CAP, &msg);
}


void __attribute__((no_instrument_function))
__cyg_profile_func_enter_c(void *function, void *caller, uintptr_t sp) //Original function placed in hooks.S, this is a callee inside of that function.
{	
	frame_size_t* fs;
	uintptr_t stack_frame_high;
	uintptr_t canary;
	fs = frame_size_find((uintptr_t)function);
	if (fs == NULL) { // In case of not finding. 
		return;
	}
	stack_frame_high = sp + fs->frame_size; // Make sure to check that these are used correctly in guards. 
	canary = sp + fs->canary_offset;
	if (fs->canary_offset == NO_CANARY) { // Should we only protect the functions with canaries that have buffers?
		canary = NO_CANARY;
	}
	
	send_frame_event(MSG_GUARD_PUSH, canary, stack_frame_high, function);
}

void __attribute__((no_instrument_function))
__cyg_profile_func_exit_c(void *function, void *caller, uintptr_t sp)
{	
	if (frame_size_find((uintptr_t)function) == NULL) {
		return;
	}
	frame_size_t* fs = frame_size_find((uintptr_t)function);
	if (fs == NULL) { // In case of not finding. 
		return;
	}
	uintptr_t stack_frame_high = sp + fs->frame_size;
	
	send_frame_event(MSG_GUARD_POP, 0, stack_frame_high, function);
}

__attribute__((no_instrument_function))
void hooks_enable(void)
{
	hooks_ready = true;
}