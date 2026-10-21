#pragma once
#include "stdint.h"

#define NO_CANARY 0

typedef struct{
	uintptr_t function;
	uint64_t frame_size;
	uint64_t canary_offset;
}frame_size_t;

frame_size_t* frame_size_find(uintptr_t fn);
