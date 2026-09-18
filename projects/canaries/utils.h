/* Helpers over the S3K API. Reference: https://marcusson.dev/s3k-api-ref */

#pragma once

#include "s3k/s3k.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* ------------------------------------------------------------------ */
/* Boot capability layout                                             */
/* ------------------------------------------------------------------ */

/** PMP frame process 0 executes from, covering app0's region. */
#define BOOT_PMP 0
/** Memory capability over the RAM every other process is carved from. */
#define RAM_MEM 1
/** Memory capability over the UART registers. */
#define UART_MEM 2
/** Memory capability over the machine timer registers. */
#define TIME_MEM 3
/** Time capability covering every slot on hart 0. */
#define HART0_TIME 4

#if S3K_HART_CNT > 1
/** Time capability covering every slot on hart 1. */
#define HART1_TIME 5
#endif
#if S3K_HART_CNT > 2
/** Time capability covering every slot on hart 2. */
#define HART2_TIME 6
/** Time capability covering every slot on hart 3. */
#define HART3_TIME 7
#endif

/** Monitor capability over every process, needed to set one up. */
#define MONITOR 8
/** Channel capability sockets are derived from. */
#define CHANNEL 9

/** First slot that boot leaves empty. */
#define FREE_CAP_BGN 10

/**
 * @brief Returned instead of an index when no free slot was found.
 *
 * One past the last valid index, so it is never mistaken for a real slot.
 */
#define UTIL_NO_CAP ((s3k_cidx_t)S3K_CAP_CNT)

/**
 * @brief Physical address of a block within a memory capability's tag.
 *
 * Memory capabilities store a tag and block numbers rather than addresses;
 * this is how `bgn`, `mrk`, and `end` are turned back into addresses.
 */
#define UTIL_TAG_BLOCK_TO_ADDR(tag, block)         \
	((((uint64_t)(tag)) << S3K_MAX_BLOCK_SIZE) \
	 + (((uint64_t)(block)) << S3K_MIN_BLOCK_SIZE))

/* ------------------------------------------------------------------ */
/* Diagnostics                                                        */
/* ------------------------------------------------------------------ */

/**
 * @brief Name of an error code, e.g. "S3K_ERR_INVALID_DERIVATION".
 *
 * @param err Error code from any `s3k_` or `util_` call.
 * @return Static string, "S3K_ERR_UNKNOWN" for a code this does not know.
 */
const char *util_err_str(s3k_err_t err);

/**
 * @brief Report a failed call, silent on success.
 *
 * Prints `what`, the error name, and the numeric code when `err` is not
 * S3K_SUCCESS. Nothing is printed on success, so it is safe to wrap every
 * call.
 *
 * @param what Label for the call, e.g. "app1 memory".
 * @param err Error code the call returned.
 * @return true when `err` is S3K_SUCCESS.
 */
bool util_check(const char *what, s3k_err_t err);

/**
 * @brief Print one capability as a single line of readable fields.
 *
 * The fields printed depend on the capability type: time ranges, memory
 * ranges and permissions, the decoded base and size of a PMP frame, monitor
 * and channel ranges, or socket mode, permissions, channel, and tag.
 *
 * @param cap Capability to print, as read by `s3k_cap_read`.
 */
void util_print_cap(s3k_cap_t cap);

/**
 * @brief Print the capability in one slot of this process's table.
 *
 * Prints the slot index followed by the capability, or the error name when
 * the slot cannot be read, so an empty slot prints as S3K_ERR_EMPTY rather
 * than being skipped.
 *
 * @param idx Slot in this process's capability table.
 */
void util_print_cap_at(s3k_cidx_t idx);

/**
 * @brief Print every non-empty slot of this process's capability table.
 *
 * Starts with the process ID, then one line per capability. Empty slots are
 * skipped.
 */
void util_dump_caps(void);

/* ------------------------------------------------------------------ */
/* Capability slots                                                   */
/* ------------------------------------------------------------------ */

/**
 * @brief Whether a slot of this process's capability table is empty.
 *
 * @param idx Slot to test.
 * @return true only when `idx` is a valid index and the slot holds nothing.
 */
bool util_cap_is_free(s3k_cidx_t idx);

/**
 * @brief First empty slot at or after `bgn`.
 *
 * @param bgn Slot to start searching from.
 * @return Slot index, or UTIL_NO_CAP when every slot from `bgn` on is taken.
 */
s3k_cidx_t util_find_free_cap_from(s3k_cidx_t bgn);

/**
 * @brief First empty slot after the boot capabilities.
 *
 * Searches from FREE_CAP_BGN, so a boot capability is never reported as free.
 *
 * @return Slot index, or UTIL_NO_CAP when the table is full.
 */
s3k_cidx_t util_find_free_cap(void);

/* ------------------------------------------------------------------ */
/* Device access                                                      */
/* ------------------------------------------------------------------ */

/**
 * @brief Give this process access to a device's registers.
 *
 * Derives a read-write PMP frame over `base` from a device memory capability,
 * loads it into a hardware PMP slot, and syncs the PMP unit, which is all
 * three steps needed before the registers can be touched.
 *
 * @param mem_idx Memory capability over the device, e.g. UART_MEM.
 * @param dst_idx Empty slot the derived PMP frame is written to.
 * @param slot Hardware PMP slot to load it into.
 * @param base Physical base address, naturally aligned to `size`.
 * @param size Region size, a power of two of at least 8 bytes.
 * @return S3K_SUCCESS, or the error from the first step that failed.
 */
s3k_err_t util_setup_uart(s3k_cidx_t mem_idx, s3k_cidx_t dst_idx,
			  s3k_pmp_slot_t slot, s3k_addr_t base,
			  s3k_addr_t size);

/* ------------------------------------------------------------------ */
/* Traps                                                              */
/* ------------------------------------------------------------------ */

/**
 * @brief Install a trap handler and the stack it runs on.
 *
 * Writes the handler to S3K_REG_TPC and the top of the stack to S3K_REG_TSP.
 * Without this, a fault suspends the process instead of entering a handler.
 *
 * @param handler Handler, declared `__attribute__((interrupt("machine")))`.
 * @param stack_base Lowest address of the handler's stack.
 * @param stack_size Size of that stack in bytes; the stack grows down from
 *        `stack_base + stack_size`.
 */
void util_setup_trap(void (*handler)(void), void *stack_base,
		     size_t stack_size);

/**
 * @brief Trap handler that prints the trap registers.
 *
 * Prints the exception program counter, stack pointer, cause, and value, then
 * returns to the faulting instruction. A fault that repeats therefore prints
 * on every attempt; replace this handler to recover instead.
 */
void util_default_trap_handler(void) __attribute__((interrupt("machine")));

/* ------------------------------------------------------------------ */
/* Starting another process                                           */
/* ------------------------------------------------------------------ */

/**
 * @brief Give another process a region of memory.
 *
 * Derives a memory capability over `[base, base + size)` from `mem_src`,
 * derives a PMP frame from it, moves that frame into the other process, and
 * loads it into one of its hardware PMP slots. The memory capability stays in
 * this process, at `*keep_mem`, so this process can revoke the grant later;
 * the other process only gets the frame.
 *
 * Needs two free slots in this process's table.
 *
 * @param pid Process to grant to.
 * @param mem_src Memory capability to carve from, e.g. RAM_MEM.
 * @param base Physical base address, naturally aligned to `size`.
 * @param size Region size, a power of two.
 * @param rwx Permissions, e.g. S3K_MEM_RWX.
 * @param dst_idx Empty slot in the other process's table for the frame.
 * @param dst_slot Hardware PMP slot in the other process to load it into.
 * @param keep_mem Out: slot in this process holding the memory capability.
 *        May be NULL, which makes the grant unrevokable by index.
 * @return S3K_SUCCESS, the error from the first step that failed, or
 *         S3K_ERR_DST_OCCUPIED when this process has no free slot.
 */
s3k_err_t util_grant_memory(s3k_pid_t pid, s3k_cidx_t mem_src, s3k_addr_t base,
			    s3k_addr_t size, s3k_rwx_t rwx, s3k_cidx_t dst_idx,
			    s3k_pmp_slot_t dst_slot, s3k_cidx_t *keep_mem);

/**
 * @brief Give another process access to a device's registers.
 *
 * Like `util_grant_memory`, but derives the PMP frame straight from a device
 * memory capability this process already holds, so nothing is kept behind and
 * the device can be shared with several processes.
 *
 * @param pid Process to grant to.
 * @param mem_src Memory capability over the device, e.g. UART_MEM.
 * @param base Physical base address, naturally aligned to `size`.
 * @param size Region size, a power of two of at least 8 bytes.
 * @param rwx Permissions, e.g. S3K_MEM_RW.
 * @param dst_idx Empty slot in the other process's table for the frame.
 * @param dst_slot Hardware PMP slot in the other process to load it into.
 * @return S3K_SUCCESS, the error from the first step that failed, or
 *         S3K_ERR_DST_OCCUPIED when this process has no free slot.
 */
s3k_err_t util_grant_device(s3k_pid_t pid, s3k_cidx_t mem_src, s3k_addr_t base,
			    s3k_addr_t size, s3k_rwx_t rwx, s3k_cidx_t dst_idx,
			    s3k_pmp_slot_t dst_slot);

/**
 * @brief Set another process's program counter and resume it.
 *
 * Both halves are needed: every process except process 0 starts suspended,
 * and the scheduler skips a suspended process even when it holds time.
 *
 * @param pid Process to start.
 * @param entry First instruction to run, which must match the ORIGIN in that
 *        application's linker script since nothing relocates the image.
 * @return S3K_SUCCESS, or the error from the write or the resume.
 */
s3k_err_t util_start(s3k_pid_t pid, s3k_addr_t entry);

/* ------------------------------------------------------------------ */
/* Scheduling                                                         */
/* ------------------------------------------------------------------ */

/**
 * @brief Give another process a slice of this process's time.
 *
 * Derives `[bgn, end)` from the time capability at `src`, moves it into the
 * other process, and syncs the schedule. Slices are carved from the source's
 * mark upwards, so `bgn` must equal that mark: grant slices in increasing
 * order, or the derivation fails with S3K_ERR_INVALID_DERIVATION.
 *
 * @param pid Process to grant to.
 * @param src Time capability to carve from, e.g. HART0_TIME.
 * @param bgn First slot of the slice, equal to `src`'s current mark.
 * @param end One past its last slot, at most `src`'s end.
 * @param dst_idx Empty slot in the other process's table.
 * @return S3K_SUCCESS, the error from the first step that failed,
 *         S3K_ERR_INVALID_CAPABILITY when `src` holds no time capability, or
 *         S3K_ERR_DST_OCCUPIED when this process has no free slot.
 */
s3k_err_t util_grant_time(s3k_pid_t pid, s3k_cidx_t src, s3k_time_slot_t bgn,
			  s3k_time_slot_t end, s3k_cidx_t dst_idx);

/**
 * @brief Give another process a whole time capability.
 *
 * Moves the capability at `src` out of this process and syncs the schedule.
 * Use it to hand over an entire hart, e.g. HART1_TIME, rather than carving a
 * slice out of the hart this process runs on.
 *
 * @param pid Process to move it to.
 * @param src Time capability in this process; it is empty afterwards.
 * @param dst_idx Empty slot in the other process's table.
 * @return S3K_SUCCESS, or the error from the move.
 */
s3k_err_t util_move_time(s3k_pid_t pid, s3k_cidx_t src, s3k_cidx_t dst_idx);

/* ------------------------------------------------------------------ */
/* IPC                                                                */
/* ------------------------------------------------------------------ */

/**
 * @brief Derive a server socket and a matching client socket.
 *
 * The server socket is derived from CHANNEL with tag 0, which is what makes
 * it the receiving end; the client socket is derived from the server with
 * `client_tag`. Both land in this process, so the client one still has to be
 * moved to the process that should use it.
 *
 * @param chan Channel to use, below S3K_CHAN_CNT.
 * @param mode IPC mode, e.g. S3K_IPC_YIELD.
 * @param perm What a message may carry, e.g. S3K_IPC_SDATA | S3K_IPC_CDATA.
 * @param client_tag Tag identifying the client, which must not be 0.
 * @param server Out: slot holding the server socket.
 * @param client Out: slot holding the client socket.
 * @return S3K_SUCCESS, the error from the first step that failed,
 *         S3K_ERR_INVALID_CAPABILITY when `client_tag` is 0, or
 *         S3K_ERR_DST_OCCUPIED when this process has no free slot.
 */
s3k_err_t util_make_socket_pair(s3k_chan_t chan, s3k_ipc_mode_t mode,
				s3k_ipc_perm_t perm, uint32_t client_tag,
				s3k_cidx_t *server, s3k_cidx_t *client);

/**
 * @brief Send a message and wait for the reply, retrying while nobody is
 *        listening.
 *
 * Repeats the call while it fails with S3K_ERR_TIMEOUT or
 * S3K_ERR_NO_RECEIVER, which is what a client needs when the server has not
 * reached its receive yet. Any other error is returned to the caller.
 *
 * @param sock Socket to send on.
 * @param msg Message to send.
 * @return The reply; check its `err` field for errors other than the two
 *         that are retried.
 */
s3k_reply_t util_sendrecv_retry(s3k_cidx_t sock, const s3k_msg_t *msg);

/**
 * @brief Handles one request in `util_server_loop` and returns the reply.
 *
 * @param request The received request, including its tag and data.
 * @param ctx The `ctx` pointer passed to `util_server_loop`.
 * @return The message to reply with.
 */
typedef s3k_msg_t (*util_handler_t)(s3k_reply_t request, void *ctx);

/**
 * @brief Serve requests on a socket forever.
 *
 * Receives a request, calls `handler`, and sends what it returns as the reply
 * to the next receive. Never returns.
 *
 * @param sock Server socket to serve on.
 * @param handler Called once per request.
 * @param ctx Passed to `handler` untouched, for its state.
 * @param recv_cap Whether to offer a free slot on every receive so clients
 *        can send capabilities as well as data.
 */
void util_server_loop(s3k_cidx_t sock, util_handler_t handler, void *ctx,
		      bool recv_cap);

/**
 * @brief Yield to another process until it blocks.
 *
 * Runs `pid` with this process's time until its state says blocked, which is
 * how a server waits for a client to reach its own receive. Busy-waits, so
 * it only terminates if `pid` does block.
 *
 * @param pid Process to wait for.
 * @return true once `pid` is blocked, false when its state cannot be read,
 *         e.g. without a monitor capability over it.
 */
bool util_wait_blocked(s3k_pid_t pid);

/* ------------------------------------------------------------------ */
/* Stack protection                                                   */
/* ------------------------------------------------------------------ */

/** Seed __stack_chk_guard. Call once, first thing in main(). */
void util_stack_protect_init(void);
