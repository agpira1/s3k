#pragma once

#include "s3k/s3k.h"

#include <stdbool.h>
#include <stdint.h>

/* Processes */
#define MONITOR_PID 1
#define VULN_PID 2

/* vuln memory layout. The stack has no permanent PMP. */
#define VULN_IMAGE_BASE 0x80030000
#define VULN_IMAGE_SIZE 0x8000
#define VULN_STACK_BASE 0x8003F000
#define VULN_STACK_TOP 0x80040000

/* vuln PMP slots */
#define VULN_SLOT_IMAGE 0
#define VULN_SLOT_UART 1
#define VULN_SLOT_WIN_FIRST 2
#define WIN_CNT 6

/* vuln capability table. Window i: cap VULN_CAP_WIN_FIRST + i, slot VULN_SLOT_WIN_FIRST + i. */
#define VULN_CAP_IMAGE 0
#define VULN_CAP_UART 1
#define VULN_CAP_TIME 2
#define VULN_CAP_SOCK 3
#define VULN_CAP_WIN_FIRST 4

/* monitor capability table */
#define MON_CAP_MEM 0
#define MON_CAP_UART 1
#define MON_CAP_TIME 2
#define MON_CAP_MONITOR 3   /* monitor cap over VULN_PID */
#define MON_CAP_STACK_MEM 4 /* memory cap over vuln's stack */
#define MON_CAP_SOCK 5	    /* server end */
#define MON_CAP_WIN_FIRST 6 /* staging, WIN_CNT entries */

/* IPC: one non-yielding socket, client sends data only */
#define GUARD_CHAN 0

/* data[0] of every message; data[1..3] as in the comments */
#define MSG_GUARD_PUSH 1 /* canary address, frame, function address */
#define MSG_GUARD_POP 2	 /* frame */
#define MSG_FAULT 3	 /* ECAUSE, EVAL, EPC */


#define GUARD_CANARY_SIZE 8
#define GUARD_RETURN_SIZE 16 /* ra + s0 (16 bytes) */

/* RISC-V mcause values */
#define CAUSE_FETCH_ACCESS 1
#define CAUSE_LOAD_ACCESS 5
#define CAUSE_STORE_ACCESS 7

typedef enum {
	GUARD_CANARY, /* [canary, canary + 8) */
	GUARD_RETURN, /* saved ra and s0, [frame - 16, frame) */
} guard_kind_t;

typedef struct {
	uint64_t bgn, end; /* [bgn, end) */
	uint64_t fn;	   /* function that owns the frame */
	guard_kind_t kind;
} guard_t;

/*
 * Monitor functions.
 */

/* Agaton: guard table and windows */
void guard_push(uint64_t canary, uint64_t frame, uint64_t fn);
void guard_pop(uint64_t frame);
const guard_t *guard_find(uint64_t addr); /* NULL if addr is not a guard */
s3k_err_t window_map(uint64_t addr);

/* Oliver: fault path. Returns true if vuln should be resumed. */
bool fault_handle(uint64_t ecause, uint64_t eval, uint64_t epc);
