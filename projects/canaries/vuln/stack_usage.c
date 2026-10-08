#include "interface.h"
#include "stdint.h"
#include "stack_usage.h"

// This file sets up a table containing offsets paired with function addresses. 
// When the fn address is sent using ipc we can connect the sp with the function stack-size to get the frame_high instead of the frame_low that is the sp. 

extern void win(void);
extern void greet(void);
extern void test(void);
extern int main(void);

typedef struct{
	uintptr_t fn;
	uint64_t size;
}frame_size_t;

// Delimitation: .su file is read manually. Otherwise the creation of this file must be scripted at compiletime to first read the .su file.
frame_size_t frame_sizes[] = {
	{(uintptr_t)&win, 16},
	{(uintptr_t)&greet, 176},
	{(uintptr_t)&test, 112},
	{(uintptr_t)&main, 32},
};

uint64_t frame_size_find(uintptr_t fn){
	;
}