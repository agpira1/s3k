#include "guards.h"
#include "interface.h"
#include "altc/altio.h"


static frame_t frames[MAX_FRAME_DEPTH];

static guard_t guards[MAX_GUARDS]; // Dont need to populate my own. Only save the guards for check if not allowed. 

static size_t frame_count; //0 by default as static.

void guard_push(uint64_t canary, uint64_t frame, uint64_t function) { //Address of the canary, address of the frame_high, address of the called function.
	
	frame_t* f; // Frame to be saved in frames. 
	guard_t* canary_guard // Canary guard to be saved in guards.
	guard_t* return_guard // Return gurad to be saved in guards.

	if (frame_count >= MAX_FRAME_DEPTH) {
		alt_puts("Guard_push: Max frame depth reached. No guard frame recorded!");
		return;
	}

	f = &frames[frame_count]; //F = object at frames[frame_count]
	f->frame = frame;
	f->canary = canary;
	f->fn = function;

	canary_guard = &guards[frame_count*2];
	canary_guard->bgn = canary;
	canary_guard->end = canary + GUARD_CANARY_SIZE;
	canary_guard->fn = function;
	canary_guard->kind = GUARD_CANARY;


	return_guard = &guards[frame_count*2+1];
	return_guard->bgn = frame - GUARD_RETURN_SIZE; //Should be the the frame_high not frame_low that is the current sp. So we add the .sn file sizes before pushing them to guard_push. 
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
};



void guard_pop(uint64_t frame) {
	frame_t* top;

	if (frame_count <= 0) {
		alt_puts("Guard_pop: No frame to pop. No guard frame removed!");
		return;
	}

	// The popped frame should always the the furthest up. 
	top = &frames[frame_count-1];
	if (top->frame != frame) {
		alt_puts("Guard_pop: Wrong frame on top. No guard frame removed!");
		return;
	}
	alt_printf( //Debug
		"POP fn=%X frame=%X resulting_depth=%D\n",
		top->fn,
		top->frame,
		(uint64_t)frame_count-1);

	frame_count--; //We overwrite the contents later if we reiterate so we don't have to zero the values and the "pop" is only to remove the value. We don't need to return it. 
};

guard_t* guard_find(uint64_t addr) {
	size_t i;
	size_t guard_count = frame_count * 2;

	for (i = 0; i < guard_count; i++) { // This checks all frame guards, needed because variables can be accessed at other parts of the stack?. The end address we record is not included as we count 0+16 which gives us the end at 16 but 16 indexes between 0-15. 
		if (addr >= guards[i].bgn &&
		    addr < guards[i].end)
			return &guards[i]; //Otherwise we return the guard object. 
	}

	return NULL;
}; /* NULL if addr is not a guard */
