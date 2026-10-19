
#include "../interface.h"

typedef struct {
	uint64_t bgn, end;
	bool used;
} window_t;

static window_t windows[WIN_CNT];
static uint64_t next_victim; // Starts on 0 because of static. 

static bool guard_check(uint64_t bgn, uint64_t end) { // Checks if a guard is between bgn and end. 
	for (uint64_t address = bgn; address < end; address+= 8) { // Checks the address in increments of 8 as both of the guards are 8+ in size it will not miss. 
		if (guard_find(address)) {
			return true;
		}
	}
	return false;
}

static s3k_err_t window_remove(uint64_t i) {
	s3k_err_t err;
	err = s3k_mon_cap_move(MON_CAP_MONITOR, VULN_PID, VULN_CAP_WIN_FIRST + i, MONITOR_PID, MON_CAP_WIN_FIRST + i);
	if (err != S3K_SUCCESS) {
		return err;
	}
	err = s3k_cap_delete(MON_CAP_WIN_FIRST + i);
	if (err != S3K_SUCCESS) {
		return err;
	}
	windows[i].used = false;
	return S3K_SUCCESS;
}

s3k_err_t new_frame_guard_check(void) { //Checks after a push or pop if the windows already present contain a new canary/guard as some windows might contain the whole stack. 
	s3k_err_t err;
	for (int i = 0; i < WIN_CNT; i++) {
		if (windows[i].used && guard_check(windows[i].bgn, windows[i].end)) {
			err = window_remove(i);
			if (err) {
				return err;
			}
		}
	}
	return S3K_SUCCESS;
}

s3k_err_t window_map(uint64_t addr) { // When pushing checks the largest window power of 2 that can be placed without hitting a guard from the address.
	s3k_err_t err;
	uint64_t bgn, size, i;
	s3k_cidx_t monitor_cap_slot, vuln_cap_slot;

	for (size = VULN_STACK_TOP - VULN_STACK_BASE; size >= 8; size /= 2) {
		bgn = addr - (addr % size); // We find the start of the size block that is aligned closest. Size is always 8 byte aligned only different size so the result will always be aligned from 0. 
		if (bgn >= VULN_STACK_BASE && bgn + size <= VULN_STACK_TOP && !guard_check(bgn, bgn + size)) {
			break; // Gives us the size value we want that has a bgn + size combo that does not contain any guards.
		}
	}
	if (size < 8) {
		return S3K_ERR_INVALID_DERIVATION;
	}
	// Find an empty window capability. 
	for (i = 0; i < WIN_CNT && windows[i].used; i++) {
		;
	}
	if (i == WIN_CNT) { //If the table is full we need to rotate. Starts on 0 by default. 
		i = next_victim;
		err = window_remove(i);
		if (err != S3K_SUCCESS) {
			return err;
		}
		next_victim = (next_victim + 1) % WIN_CNT;
	}

	monitor_cap_slot = MON_CAP_WIN_FIRST + i;
	vuln_cap_slot = VULN_CAP_WIN_FIRST + i;
	err = s3k_cap_derive(MON_CAP_STACK_MEM, monitor_cap_slot, s3k_mk_pmp(s3k_napot_encode(bgn, size), S3K_MEM_RW)); //Derives a RW PMP from bgn of size and gives ownership to the monitor at capability slot monitor_cap_slot.
	if (err != S3K_SUCCESS) {
		return err;
	}

	err = s3k_mon_cap_move(MON_CAP_MONITOR, MONITOR_PID, monitor_cap_slot, VULN_PID, vuln_cap_slot); // Moves the pmp capability to the vuln at place vuln_cap_slot. 
	
	if (err != S3K_SUCCESS) {
		s3k_cap_delete(monitor_cap_slot); //If the cap move and/or load fail we need to delete the earlier capability. Therefore the error doesn't return instantly.
		return err;
	}

	err = s3k_mon_pmp_load(MON_CAP_MONITOR, VULN_PID, vuln_cap_slot, VULN_SLOT_WIN_FIRST + i); // Moves the PMP from the capabilities into the pmp table in the vuln process.
	
	if (err != S3K_SUCCESS) {
		s3k_mon_cap_move(MON_CAP_MONITOR, VULN_PID, vuln_cap_slot, MONITOR_PID, monitor_cap_slot);
		s3k_cap_delete(monitor_cap_slot);
		return err;
	}

	windows[i].bgn = bgn;
	windows[i].end = bgn + size;
	windows[i].used = true;

	return S3K_SUCCESS;
}