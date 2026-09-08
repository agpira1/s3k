/* Helpers shared by the applications in this project.
 *
 * Nothing here is kernel API. Every function in this file is a thin wrapper
 * over calls declared in common/inc/s3k/, and every constant is either a copy
 * of a platform value or a convention this project picked. The kernel does not
 * know these names exist.
 *
 * This is a real translation unit (utils.c), not a header full of definitions.
 * projects/build.mk compiles ${PROGRAM}/*.c, so an application picks it up by
 * keeping a copy in its own directory, or by adding utils.c to its sources.
 */
#pragma once

#include "s3k/s3k.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* ------------------------------------------------------------------ */
/* Boot capability layout                                             */
/* ------------------------------------------------------------------ */

/* Indices 0-9 of process 0 are filled by the kernel at boot from INIT_CAPS in
 * common/inc/plat/<platform>.h. These names mirror that array. They are a
 * platform convention, not properties of the capability types, and they differ
 * between platforms: qemu_virt has one hart and one time capability, while
 * qemu_virt4 has four. Check the platform header before trusting an index.
 *
 * Every other process starts with an empty capability table. See
 * kernel/src/proc.c:proc_init, which special-cases process 0 and leaves every
 * other process suspended with a program counter of zero. */
#define BOOT_PMP 0  /* PMP frame covering this program; already in slot 0 */
#define RAM_MEM 1   /* free RAM above this program, RWX                   */
#define UART_MEM 2  /* memory-mapped device range containing the UART, RW */
#define TIME_MEM 3  /* timer registers, read only                         */
#define HART0_TIME 4

#if S3K_HART_CNT > 1
#define HART1_TIME 5
#endif
#if S3K_HART_CNT > 2
#define HART2_TIME 6
#define HART3_TIME 7
#endif

#define MONITOR 8 /* control over every configured PID  */
#define CHANNEL 9 /* pool of IPC channel numbers        */

/* First capability index not claimed by INIT_CAPS. Slots from here to
 * S3K_CAP_CNT - 1 are free for this process to use. */
#define FREE_CAP_BGN 10

/* Returned by util_find_free_cap when the table is full. Not a usable index:
 * S3K_CAP_CNT is one past the last slot. Zero would be wrong as a sentinel,
 * because index 0 holds the PMP frame this code is executing from. */
#define UTIL_NO_CAP ((s3k_cidx_t)S3K_CAP_CNT)

/* Reconstruct an address from the tag and block number stored in a memory
 * capability. A memory capability does not hold an address; it holds a 128 MiB
 * tag window plus 4 KiB block offsets within it. See S3K_MIN_BLOCK_SIZE and
 * S3K_MAX_BLOCK_SIZE in common/inc/s3k/types.h. */
#define UTIL_TAG_BLOCK_TO_ADDR(tag, block)                \
	((((uint64_t)(tag)) << S3K_MAX_BLOCK_SIZE)        \
	 + (((uint64_t)(block)) << S3K_MIN_BLOCK_SIZE))

/* ------------------------------------------------------------------ */
/* Diagnostics                                                        */
/* ------------------------------------------------------------------ */

/* Name of an error code, e.g. "S3K_ERR_INVALID_DERIVATION". Returns
 * "S3K_ERR_UNKNOWN" for a value outside the enum. Never returns NULL. */
const char *util_err_str(s3k_err_t err);

/* Report err if it is not S3K_SUCCESS, and return true when the call
 * succeeded. Prints nothing on success, so it is safe to wrap every call:
 *
 *     if (!util_check("derive uart", s3k_cap_derive(...)))
 *             return;
 *
 * Requires a working UART; before util_setup_uart has run there is nowhere to
 * print and a failure is silent. */
bool util_check(const char *what, s3k_err_t err);

/* Print one capability in a human-readable form. Passed by value: an
 * s3k_cap_t is exactly 8 bytes. */
void util_print_cap(s3k_cap_t cap);

/* Read the capability at idx and print it, or report why it could not be
 * read. Unlike the tutorial version this does not spin forever on failure. */
void util_print_cap_at(s3k_cidx_t idx);

/* Print every non-empty slot in this process's capability table. */
void util_dump_caps(void);

/* ------------------------------------------------------------------ */
/* Capability slots                                                   */
/* ------------------------------------------------------------------ */

/* True when idx holds no capability. An out-of-range index is not free. */
bool util_cap_is_free(s3k_cidx_t idx);

/* Lowest free slot at or above bgn, or UTIL_NO_CAP if there is none.
 *
 * This does not reserve anything. Two calls with no derive in between return
 * the same index, so either derive immediately or pass a higher bgn on the
 * second call. */
s3k_cidx_t util_find_free_cap_from(s3k_cidx_t bgn);

/* util_find_free_cap_from(FREE_CAP_BGN). */
s3k_cidx_t util_find_free_cap(void);

/* ------------------------------------------------------------------ */
/* Device access                                                      */
/* ------------------------------------------------------------------ */

/* Claim read/write access to a device range so that alt_puts and alt_printf
 * produce output. Three steps, all of which are required:
 *
 *   1. derive a PMP frame out of the memory capability at mem_idx
 *   2. load it into this process's shadow PMP state
 *   3. synchronise the shadow state into the hardware CSRs
 *
 * Skipping step 3 leaves the grant invisible to the processor and the next
 * store to the UART traps.
 *
 * size must be a power of two of at least 8, and base must be aligned to it,
 * because a PMP frame is NAPOT-encoded. 0x8 covers the NS16550A registers;
 * qemu_virt4 needs 0x2000 to reach virtio as well.
 *
 * The memory capability at mem_idx is locked by this call and cannot be
 * subdivided further until the derived frame is revoked. */
s3k_err_t util_setup_uart(s3k_cidx_t mem_idx, s3k_cidx_t dst_idx,
			  s3k_pmp_slot_t slot, s3k_addr_t base,
			  s3k_addr_t size);

/* ------------------------------------------------------------------ */
/* Traps                                                              */
/* ------------------------------------------------------------------ */

/* Install a trap handler and the stack it runs on. stack_base is the low
 * address of the buffer; the stack pointer is set to its top, because RISC-V
 * stacks grow down. Neither pointer is retained by the kernel beyond the
 * register write, but the buffer must outlive every trap, so give it static
 * storage duration. */
void util_setup_trap(void (*handler)(void), void *stack_base,
		     size_t stack_size);

/* Prints the trap cause and resumes at the faulting instruction.
 *
 * The interrupt attribute makes the compiler emit mret instead of ret. The
 * kernel catches an mret from user mode and restores pc from epc and sp from
 * esp, so returning normally re-executes the instruction that trapped. That
 * loops forever unless the handler changes epc, which makes this useful for
 * seeing what went wrong and useless as a recovery strategy. */
void util_default_trap_handler(void) __attribute__((interrupt("machine")));

/* ------------------------------------------------------------------ */
/* Starting another process                                           */
/* ------------------------------------------------------------------ */

/* Carve [base, base + size) out of the memory capability at mem_src, hand the
 * target process a PMP frame for it, and load that frame into one of the
 * target's hardware slots.
 *
 * The memory capability stays in this process's table, at the index written to
 * *keep_mem. That is deliberate: the deed stays with the parent so it can
 * revoke the grant later, while the child receives only the gate key. Pass
 * NULL if you do not intend to revoke, but the slot is still consumed.
 *
 * size must be a power of two of at least 4 KiB and base aligned to it. */
s3k_err_t util_grant_memory(s3k_pid_t pid, s3k_cidx_t mem_src, s3k_addr_t base,
			    s3k_addr_t size, s3k_rwx_t rwx, s3k_cidx_t dst_idx,
			    s3k_pmp_slot_t dst_slot, s3k_cidx_t *keep_mem);

/* Give the target process a PMP frame over an existing memory capability,
 * without carving a new slice first. Use this to share a device the parent
 * already has a memory capability for, such as UART_MEM. */
s3k_err_t util_grant_device(s3k_pid_t pid, s3k_cidx_t mem_src, s3k_addr_t base,
			    s3k_addr_t size, s3k_rwx_t rwx, s3k_cidx_t dst_idx,
			    s3k_pmp_slot_t dst_slot);

/* Set the target's program counter and take it out of suspension.
 *
 * The resume matters. proc_init leaves every process except process 0
 * suspended, and a suspended process is skipped by the scheduler even when it
 * holds time. Setting the program counter is not enough on its own; the
 * tutorials call s3k_mon_resume separately and it is easy to forget.
 *
 * The target's stack pointer needs no setup: start.S loads it from
 * __stack_pointer in the linker script. */
s3k_err_t util_start(s3k_pid_t pid, s3k_addr_t entry);

/* ------------------------------------------------------------------ */
/* Scheduling                                                         */
/* ------------------------------------------------------------------ */

/* Give the target process slots [bgn, end) of the hart time capability at
 * src, keeping the rest. The parent keeps src. */
s3k_err_t util_grant_time(s3k_pid_t pid, s3k_cidx_t src, s3k_time_slot_t bgn,
			  s3k_time_slot_t end, s3k_cidx_t dst_idx);

/* Give the target process an entire hart by moving the capability at src out
 * of this table. After this the parent has no time on that hart. */
s3k_err_t util_move_time(s3k_pid_t pid, s3k_cidx_t src, s3k_cidx_t dst_idx);

/* ------------------------------------------------------------------ */
/* IPC                                                                */
/* ------------------------------------------------------------------ */

/* Create a connected socket pair on an IPC channel, leaving both ends in this
 * process's table at *server and *client. Move one end to the peer with
 * s3k_mon_cap_move.
 *
 * The two ends are not symmetric. The server socket is derived from the
 * channel capability with tag 0; the client socket is derived from the server
 * socket with a non-zero tag. A socket with tag 0 receives, a socket with a
 * non-zero tag sends. Deriving both from the channel does not work.
 *
 * mode is S3K_IPC_YIELD or S3K_IPC_NOYIELD. perm is a bitwise or of
 * S3K_IPC_SDATA, S3K_IPC_SCAP, S3K_IPC_CDATA and S3K_IPC_CCAP. */
s3k_err_t util_make_socket_pair(s3k_chan_t chan, s3k_ipc_mode_t mode,
				s3k_ipc_perm_t perm, uint32_t client_tag,
				s3k_cidx_t *server, s3k_cidx_t *client);

/* Send and wait for a reply, retrying while the peer is not yet listening.
 *
 * S3K_ERR_TIMEOUT is the ordinary outcome of parking on a socket, not a
 * failure: it means this process ran out of time before the peer answered.
 * Retrying is how a client waits. Any other error is returned to the caller. */
s3k_reply_t util_sendrecv_retry(s3k_cidx_t sock, const s3k_msg_t *msg);

/* Handler for util_server_loop. Receives the request and returns the reply
 * that should be sent back. ctx is passed through untouched. */
typedef s3k_msg_t (*util_handler_t)(s3k_reply_t request, void *ctx);

/* Serve requests forever. When recv_cap is true, a free capability slot is
 * reserved before each receive so that the peer is allowed to send a
 * capability; the received capability arrives in request.cap. Never returns. */
void util_server_loop(s3k_cidx_t sock, util_handler_t handler, void *ctx,
		      bool recv_cap);

/* Spin until the target process is blocked on IPC, yielding to it in between
 * so that it can get there. Returns false if the state could not be read.
 *
 * Note that s3k_is_blocked in common/inc/s3k/util.h reports the channel with
 * a different shift than kernel/src/proc.c uses to encode it, so this checks
 * S3K_PSF_BLOCKED directly rather than trusting the decoded channel. */
bool util_wait_blocked(s3k_pid_t pid);
