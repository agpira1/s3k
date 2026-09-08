/* Helpers over the S3K API. Reference: https://marcusson.dev/s3k-api-ref */

#pragma once

#include "s3k/s3k.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* ------------------------------------------------------------------ */
/* Boot capability layout                                             */
/* ------------------------------------------------------------------ */

#define BOOT_PMP 0
#define RAM_MEM 1
#define UART_MEM 2
#define TIME_MEM 3
#define HART0_TIME 4

#if S3K_HART_CNT > 1
#define HART1_TIME 5
#endif
#if S3K_HART_CNT > 2
#define HART2_TIME 6
#define HART3_TIME 7
#endif

#define MONITOR 8
#define CHANNEL 9

#define FREE_CAP_BGN 10

#define UTIL_NO_CAP ((s3k_cidx_t)S3K_CAP_CNT)

#define UTIL_TAG_BLOCK_TO_ADDR(tag, block)                \
	((((uint64_t)(tag)) << S3K_MAX_BLOCK_SIZE)        \
	 + (((uint64_t)(block)) << S3K_MIN_BLOCK_SIZE))

/* ------------------------------------------------------------------ */
/* Diagnostics                                                        */
/* ------------------------------------------------------------------ */

const char *util_err_str(s3k_err_t err);

bool util_check(const char *what, s3k_err_t err);

void util_print_cap(s3k_cap_t cap);

void util_print_cap_at(s3k_cidx_t idx);

void util_dump_caps(void);

/* ------------------------------------------------------------------ */
/* Capability slots                                                   */
/* ------------------------------------------------------------------ */

bool util_cap_is_free(s3k_cidx_t idx);

s3k_cidx_t util_find_free_cap_from(s3k_cidx_t bgn);

s3k_cidx_t util_find_free_cap(void);

/* ------------------------------------------------------------------ */
/* Device access                                                      */
/* ------------------------------------------------------------------ */

s3k_err_t util_setup_uart(s3k_cidx_t mem_idx, s3k_cidx_t dst_idx,
			  s3k_pmp_slot_t slot, s3k_addr_t base,
			  s3k_addr_t size);

/* ------------------------------------------------------------------ */
/* Traps                                                              */
/* ------------------------------------------------------------------ */

void util_setup_trap(void (*handler)(void), void *stack_base,
		     size_t stack_size);

void util_default_trap_handler(void) __attribute__((interrupt("machine")));

/* ------------------------------------------------------------------ */
/* Starting another process                                           */
/* ------------------------------------------------------------------ */

s3k_err_t util_grant_memory(s3k_pid_t pid, s3k_cidx_t mem_src, s3k_addr_t base,
			    s3k_addr_t size, s3k_rwx_t rwx, s3k_cidx_t dst_idx,
			    s3k_pmp_slot_t dst_slot, s3k_cidx_t *keep_mem);

s3k_err_t util_grant_device(s3k_pid_t pid, s3k_cidx_t mem_src, s3k_addr_t base,
			    s3k_addr_t size, s3k_rwx_t rwx, s3k_cidx_t dst_idx,
			    s3k_pmp_slot_t dst_slot);

s3k_err_t util_start(s3k_pid_t pid, s3k_addr_t entry);

/* ------------------------------------------------------------------ */
/* Scheduling                                                         */
/* ------------------------------------------------------------------ */

s3k_err_t util_grant_time(s3k_pid_t pid, s3k_cidx_t src, s3k_time_slot_t bgn,
			  s3k_time_slot_t end, s3k_cidx_t dst_idx);

s3k_err_t util_move_time(s3k_pid_t pid, s3k_cidx_t src, s3k_cidx_t dst_idx);

/* ------------------------------------------------------------------ */
/* IPC                                                                */
/* ------------------------------------------------------------------ */

s3k_err_t util_make_socket_pair(s3k_chan_t chan, s3k_ipc_mode_t mode,
				s3k_ipc_perm_t perm, uint32_t client_tag,
				s3k_cidx_t *server, s3k_cidx_t *client);

s3k_reply_t util_sendrecv_retry(s3k_cidx_t sock, const s3k_msg_t *msg);

typedef s3k_msg_t (*util_handler_t)(s3k_reply_t request, void *ctx);

void util_server_loop(s3k_cidx_t sock, util_handler_t handler, void *ctx,
		      bool recv_cap);

bool util_wait_blocked(s3k_pid_t pid);
