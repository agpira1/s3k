#include "../interface.h"
#include "altc/altio.h"

/* ponytail: logs and stops on every fault; classification comes in phase 4. */
bool fault_handle(uint64_t ecause, uint64_t eval, uint64_t epc)
{
	alt_printf("monitor: fault cause:%X addr:%X pc:%X\n", ecause, eval,
		   epc);
	return false;
}
