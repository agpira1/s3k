#include "guards.h"
#include "interface.h"
#include "altc/altio.h"
#include "stdint.h"
#include "windows.h"


static uintptr_t frames[MAX_FRAME_DEPTH];

static guard_t guards[MAX_GUARDS]; // Dont need to populate my own. Only save the guards for check if not allowed. 

static uint64_t frame_count; //0 by default as static.

bool guard_push(uintptr_t canary, uintptr_t frame, uintptr_t function) { //Address of the canary, address of the frame_high, address of the called function.
	
	guard_t* canary_guard; // Canary guard to be saved in guards.
	guard_t* return_guard; // Return guard to be saved in guards.

	if (frame_count >= MAX_FRAME_DEPTH) {
		alt_puts("Guard_push: Max frame depth reached. No guard frame recorded!");
		return false;
	}

	frames[frame_count] = frame;

	canary_guard = &guards[frame_count*2];
	canary_guard->bgn = canary;
	canary_guard->end = canary + GUARD_CANARY_SIZE;
	canary_guard->fn = function;
	canary_guard->kind = GUARD_CANARY;
	if (canary == 0) {
		canary_guard->end = canary;
	}

	return_guard = &guards[frame_count*2+1];
	return_guard->bgn = frame - GUARD_RETURN_SIZE; 
	return_guard->end = frame;
	return_guard->fn = function;
	return_guard->kind = GUARD_RETURN;

	frame_count++;
	alt_printf( //Debug
		"PUSH fn=%X frame=%X canary=%X resulting_depth=%D\n",
		function,
		frame,
		canary,
		(uint64_t)frame_count);
	
	return true;
};



bool guard_pop(uintptr_t frame) {

	if (frame_count == 0) {
		alt_puts("Guard_pop: No frame to pop. No guard frame removed!");
		return false;
	}

	// The popped frame should always be the furthest up. 
	uintptr_t top = frames[frame_count-1];
	if (top != frame) {
		alt_puts("Guard_pop: Wrong frame on top. No guard frame removed!");
		return false;
	}
	alt_printf( //Debug
		"POP fn=%X frame=%X resulting_depth=%D\n",
		(uint64_t)guards[(frame_count - 1) * 2].fn,
        (uint64_t)top,
		(uint64_t)frame_count-1);

	frame_count--; //We overwrite the contents later if we reiterate so we don't have to zero the values and the "pop" is only to remove the value. We don't need to return it. 
	return true;
};

guard_t* guard_find(uintptr_t addr) {
	uint64_t i;
	uint64_t guard_count = frame_count * 2;

	for (i = 0; i < guard_count; i++) { // This checks all frame guards, needed because variables can be accessed at other parts of the stack?. The end address we record is not included as we count 0+16 which gives us the end at 16 but 16 indexes between 0-15. 
		if (addr >= guards[i].bgn && addr < guards[i].end)
			return &guards[i]; //Otherwise we return the guard object. 
	}

	return NULL;
}; /* NULL if addr does not contain a guard */
