#pragma once
#include <stdint.h>

enum frame_event_type { // FRAME_ENTER and EXIT events are integers.
	FRAME_ENTER = 1,
	FRAME_EXIT  = 2,
	FRAME_TOP   = 3,
};

struct frame_event {
	uint64_t type; //S3K IPC msg contains 4 64-bit words. 
	uint64_t function;
	uint64_t caller;
	uint64_t sp;
};