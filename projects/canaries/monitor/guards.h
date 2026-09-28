#pragma once

#include "../frame_msg.h"
#include <stdint.h>


struct guard_frame {
	uintptr_t function; //From GCC finstrument-functions. 
	uintptr_t caller; //From GCC finstrument-functions. 

	uintptr_t sp_at_entry; //SP at entry at the time of finstrument-function. 

	// NOT USED YET
	uintptr_t frame_start; // TODO: Implement logic to populate the fields.
	uintptr_t frame_end;

	uintptr_t canary_addr;
	uintptr_t canary_value;

	uintptr_t saved_ra_addr;
	uintptr_t saved_s0_addr;

};