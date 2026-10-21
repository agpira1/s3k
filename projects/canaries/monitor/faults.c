#include "../interface.h"
#include "altc/altio.h"

static bool in_stack(uint64_t addr)
{
	return addr >= VULN_STACK_BASE && addr < VULN_STACK_TOP;
}

static bool stop(const char *why, uint64_t ecause, uint64_t eval,
		 uint64_t epc)
{
	alt_printf("monitor: %s cause:%X addr:%X pc:%X\n", why, ecause, eval,
		   epc);
	return false;
}

bool fault_handle(uint64_t ecause, uint64_t eval, uint64_t epc)
{
	static uint64_t last_eval, last_epc, faults;
	const guard_t *g;

	if (ecause != CAUSE_FETCH_ACCESS && ecause != CAUSE_LOAD_ACCESS
	    && ecause != CAUSE_STORE_ACCESS)
		return stop("crash", ecause, eval, epc);

	// Windows are RW only; mapping one for a fetch would fault forever.
	if (ecause == CAUSE_FETCH_ACCESS && in_stack(eval))
		return stop("attack: fetch from stack", ecause, eval, epc);

	g = guard_find(eval);
	if (g) {
		alt_printf("monitor: attack: %s fn:%X\n",
			   g->kind == GUARD_CANARY ? "canary" : "return",
			   g->fn);
		return stop("attack", ecause, eval, epc);
	}

	if (!in_stack(eval))
		return stop("out of bounds", ecause, eval, epc);

	if (eval == last_eval && epc == last_epc)
		return stop("fault loop", ecause, eval, epc);
	last_eval = eval;
	last_epc = epc;

	if (window_map(eval) != S3K_SUCCESS)
		return stop("window_map failed", ecause, eval, epc);
	alt_printf("monitor: fault %D: window for addr:%X pc:%X\n", ++faults,
		   eval, epc);
	return true;
}
