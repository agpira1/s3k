#pragma once

#include <stdint.h>

#define MAX_FRAME_DEPTH 64
#define MAX_GUARDS (MAX_FRAME_DEPTH * 2)

typedef struct {
	uint64_t frame; //SP + frame size, if we count from the top or bottom might not matter but most of the things we need are at the top so less offset. 
	uint64_t canary; //Address of the canary. 
	uint64_t fn; //Function being called. 
}frame_t;