#include "interface.h"
#include "stdint.h"
#include "stack_usage.h"

// This file sets up a table containing offsets paired with function addresses. 
// When the fn address is sent using ipc we can connect the sp with the function stack-size to get the frame_high instead of the frame_low that is the sp. 



extern void win(void);
extern void greet(void);
extern void test(void);
extern int main(void);
extern void test2(void);

// Check for frame_size in the .su files, check for canary offset manually in objdump.
frame_size_t frame_sizes[] = { 
	{.function = (uintptr_t)&win, .frame_size = 16, .canary_offset = NO_CANARY},
    {.function = (uintptr_t)&greet, .frame_size = 176, .canary_offset = 136},
	{.function = (uintptr_t)&test, .frame_size = 112, .canary_offset = 72},	
	{.function = (uintptr_t)&main, .frame_size = 32, .canary_offset = NO_CANARY}, 
	{.function = (uintptr_t)&test2, .frame_size = 32, .canary_offset = NO_CANARY},};

frame_size_t* __attribute__((no_instrument_function))
frame_size_find(uintptr_t function){ //Returns entire item to get both values. 
	uint64_t i;
	for (i = 0; i < sizeof(frame_sizes) / sizeof(frame_sizes[0]); i++) {
		if (frame_sizes[i].function == function) {
			return &frame_sizes[i];
		}
	}
	return NULL;
}