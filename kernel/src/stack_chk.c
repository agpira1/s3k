/* See LICENSE file for copyright and license details. */
#include "drivers/time.h"
#include "csr.h"
#include "kprintf.h"
#include "stack_chk.h"
#include "wfi.h"

#include <stdint.h>

uintptr_t __stack_chk_guard;

__attribute__((used, optimize("no-stack-protector"))) void
stack_chk_guard_init(void)
{
	uint64_t r = time_get();
	r ^= (uint64_t)&__stack_chk_guard;
	r ^= (uint64_t)csrr(mhartid) << 56;
	r *= 0xff51afd7ed558ccdULL;
	r ^= r >> 33;
	__stack_chk_guard = r & ~0xffULL;
}

__attribute__((noreturn, used)) void __stack_chk_fail(void)
{
	kprintf(0, "# KERNEL STACK SMASHING DETECTED -- halting\n");
	for (;;)
		wfi();
}
