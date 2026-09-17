#include "altc/altio.h"
#include "s3k/s3k.h"

#include "../../tutorial-commons/utils.h"

char trap_stack[1024];

int main(void)
{
	// Setup UART access
	setup_uart();

	setup_trap(default_trap_handler, trap_stack, 1024);

	// Write hello world.
	alt_printf("Hello, world\n");

	debug_capability_from_idx(RAM_MEM); // Memory rwx:7 lock:0 bgn:80020000 mrk:80020000 end:88000000

	uint32_t free_cap = find_free_cap(); // Finds a free slot in the table. Nothing else, no allocation. 
	// Free_cap is a capability slot destination. For this program. 

	log_sys("Test1", s3k_cap_derive(RAM_MEM, free_cap, s3k_mk_memory(APP_1_BASE_ADDR, APP_1_BASE_ADDR + APP_1_SIZE, S3K_MEM_RWX)));
	// Gives memory capability to this program derived from RAM_MEM (Highest authority in memory?). Given the allocation and permissions specified.

	// We also need a hardware protection capability. Physical Memory Protection PMP. Same thing there. 

	uint32_t free_cap_pmp = find_free_cap();

	uint64_t pmp_addr = s3k_napot_encode(APP_1_BASE_ADDR, APP_1_SIZE);
	// Memory capabilities are stored separate and are targetting RISC-V not S3K i guess. 

	log_sys("Test2", s3k_cap_derive(free_cap, free_cap_pmp, s3k_mk_pmp(pmp_addr, S3K_MEM_RW))); // Returns derivation error like we reasoned. So even within the capabilites there are tiers. One program needs both memory allocated through software and hardware, hardware comes after as it is deeper down in risc-V?

	s3k_pmp_load(free_cap_pmp, 2); // Why slot 2?

	s3k_sync_mem();

	*((uint64_t *)(APP_1_BASE_ADDR)) = 10;
	alt_printf("Successfully wrote in random memory 0x%X\n", *((uint64_t *)(APP_1_BASE_ADDR)));

	
}
