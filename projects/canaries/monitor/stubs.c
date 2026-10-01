/* Weak stand-ins for guards.c and windows.c; the real ones override them. */

#include "../interface.h"

__attribute__((weak)) void guard_push(uint64_t canary, uint64_t frame,
				      uint64_t fn)
{
}

__attribute__((weak)) void guard_pop(uint64_t frame)
{
}

__attribute__((weak)) const guard_t *guard_find(uint64_t addr)
{
	return NULL;
}

__attribute__((weak)) s3k_err_t window_map(uint64_t addr)
{
	return S3K_ERR_EMPTY;
}
