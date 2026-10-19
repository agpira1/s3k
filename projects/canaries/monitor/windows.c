/* Temporary window_map */

#include "../interface.h"

static struct {
	uint64_t bgn, end;
	bool used;
} wins[WIN_CNT];

static int next_victim;

static bool overlaps_guard(uint64_t bgn, uint64_t end)
{
	for (uint64_t a = bgn; a < end; a += 8)
		if (guard_find(a) || guard_find(a + 7))
			return true;
	return false;
}

static void evict(int i)
{
	// Moving a loaded PMP cap unloads it; deleting frees the index.
	s3k_mon_cap_move(MON_CAP_MONITOR, VULN_PID, VULN_CAP_WIN_FIRST + i,
			 MONITOR_PID, MON_CAP_WIN_FIRST + i);
	s3k_cap_delete(MON_CAP_WIN_FIRST + i);
	wins[i].used = false;
}

void window_invalidate(void)
{
	for (int i = 0; i < WIN_CNT; i++)
		if (wins[i].used && overlaps_guard(wins[i].bgn, wins[i].end))
			evict(i);
}

s3k_err_t window_map(uint64_t addr)
{
	uint64_t size, bgn;
	s3k_cidx_t staging, cap;
	s3k_err_t err;
	int i;

	// Largest NAPOT region around addr inside the stack with no guard.
	for (size = VULN_STACK_TOP - VULN_STACK_BASE; size >= 8; size /= 2) {
		bgn = addr & ~(size - 1);
		if (bgn >= VULN_STACK_BASE && bgn + size <= VULN_STACK_TOP
		    && !overlaps_guard(bgn, bgn + size))
			break;
	}
	if (size < 8)
		return S3K_ERR_INVALID_DERIVATION;

	for (i = 0; i < WIN_CNT && wins[i].used; i++)
		;
	if (i == WIN_CNT) {
		i = next_victim;
		next_victim = (next_victim + 1) % WIN_CNT;
		evict(i);
	}

	staging = MON_CAP_WIN_FIRST + i;
	cap = VULN_CAP_WIN_FIRST + i;
	err = s3k_cap_derive(MON_CAP_STACK_MEM, staging,
			     s3k_mk_pmp(s3k_napot_encode(bgn, size),
					S3K_MEM_RW));
	if (!err)
		err = s3k_mon_cap_move(MON_CAP_MONITOR, MONITOR_PID, staging,
				       VULN_PID, cap);
	if (!err)
		err = s3k_mon_pmp_load(MON_CAP_MONITOR, VULN_PID, cap,
				       VULN_SLOT_WIN_FIRST + i);
	if (err) {
		s3k_cap_delete(staging); // no-op if the move already happened
		return err;
	}

	wins[i].bgn = bgn;
	wins[i].end = bgn + size;
	wins[i].used = true;
	return S3K_SUCCESS;
}
