/* See utils.h for what each function does and why. */

#include "utils.h"

#include "altc/altio.h"

/* ------------------------------------------------------------------ */
/* Diagnostics                                                        */
/* ------------------------------------------------------------------ */

static const char *const err_names[] = {
	[S3K_SUCCESS] = "S3K_SUCCESS",
	[S3K_ERR_EMPTY] = "S3K_ERR_EMPTY",
	[S3K_ERR_SRC_EMPTY] = "S3K_ERR_SRC_EMPTY",
	[S3K_ERR_DST_OCCUPIED] = "S3K_ERR_DST_OCCUPIED",
	[S3K_ERR_INVALID_INDEX] = "S3K_ERR_INVALID_INDEX",
	[S3K_ERR_INVALID_DERIVATION] = "S3K_ERR_INVALID_DERIVATION",
	[S3K_ERR_INVALID_MONITOR] = "S3K_ERR_INVALID_MONITOR",
	[S3K_ERR_INVALID_PID] = "S3K_ERR_INVALID_PID",
	[S3K_ERR_INVALID_STATE] = "S3K_ERR_INVALID_STATE",
	[S3K_ERR_INVALID_PMP] = "S3K_ERR_INVALID_PMP",
	[S3K_ERR_INVALID_SLOT] = "S3K_ERR_INVALID_SLOT",
	[S3K_ERR_INVALID_SOCKET] = "S3K_ERR_INVALID_SOCKET",
	[S3K_ERR_INVALID_SYSCALL] = "S3K_ERR_INVALID_SYSCALL",
	[S3K_ERR_INVALID_REGISTER] = "S3K_ERR_INVALID_REGISTER",
	[S3K_ERR_INVALID_CAPABILITY] = "S3K_ERR_INVALID_CAPABILITY",
	[S3K_ERR_NO_RECEIVER] = "S3K_ERR_NO_RECEIVER",
	[S3K_ERR_PREEMPTED] = "S3K_ERR_PREEMPTED",
	[S3K_ERR_TIMEOUT] = "S3K_ERR_TIMEOUT",
	[S3K_ERR_SUSPENDED] = "S3K_ERR_SUSPENDED",
};

const char *util_err_str(s3k_err_t err)
{
	if (err >= sizeof(err_names) / sizeof(err_names[0])
	    || err_names[err] == NULL)
		return "S3K_ERR_UNKNOWN";
	return err_names[err];
}

bool util_check(const char *what, s3k_err_t err)
{
	if (err == S3K_SUCCESS)
		return true;
	alt_printf("%s: %s (%D)\n", what, util_err_str(err), (uint64_t)err);
	return false;
}

/* alt_printf understands %c %s %x %X %d %D and %% only. There is no width, no
 * precision, and no default case in the format loop, so an unknown specifier
 * such as %Z prints nothing and consumes no argument. %x reads an unsigned
 * int, %X an unsigned long; bitfields promote to int, so every 64-bit slot is
 * cast explicitly here rather than relying on the calling convention to
 * widen it. */
void util_print_cap(s3k_cap_t cap)
{
	switch (cap.type) {
	case S3K_CAPTY_NONE:
		alt_printf("none\n");
		break;
	case S3K_CAPTY_TIME:
		alt_printf("time     hart:%X bgn:%X mrk:%X end:%X\n",
			   (uint64_t)cap.time.hart, (uint64_t)cap.time.bgn,
			   (uint64_t)cap.time.mrk, (uint64_t)cap.time.end);
		break;
	case S3K_CAPTY_MEMORY:
		alt_printf("memory   rwx:%X lck:%X bgn:%X mrk:%X end:%X\n",
			   (uint64_t)cap.mem.rwx, (uint64_t)cap.mem.lck,
			   UTIL_TAG_BLOCK_TO_ADDR(cap.mem.tag, cap.mem.bgn),
			   UTIL_TAG_BLOCK_TO_ADDR(cap.mem.tag, cap.mem.mrk),
			   UTIL_TAG_BLOCK_TO_ADDR(cap.mem.tag, cap.mem.end));
		break;
	case S3K_CAPTY_PMP: {
		s3k_addr_t base;
		s3k_addr_t size;

		s3k_napot_decode(cap.pmp.addr, &base, &size);
		alt_printf("pmp      rwx:%X used:%X slot:%X addr:%X "
			   "base:%X size:%X\n",
			   (uint64_t)cap.pmp.rwx, (uint64_t)cap.pmp.used,
			   (uint64_t)cap.pmp.slot, (uint64_t)cap.pmp.addr,
			   base, size);
		break;
	}
	case S3K_CAPTY_MONITOR:
		alt_printf("monitor  bgn:%X mrk:%X end:%X\n",
			   (uint64_t)cap.mon.bgn, (uint64_t)cap.mon.mrk,
			   (uint64_t)cap.mon.end);
		break;
	case S3K_CAPTY_CHANNEL:
		alt_printf("channel  bgn:%X mrk:%X end:%X\n",
			   (uint64_t)cap.chan.bgn, (uint64_t)cap.chan.mrk,
			   (uint64_t)cap.chan.end);
		break;
	case S3K_CAPTY_SOCKET:
		alt_printf("socket   mode:%X perm:%X chan:%X tag:%X\n",
			   (uint64_t)cap.sock.mode, (uint64_t)cap.sock.perm,
			   (uint64_t)cap.sock.chan, (uint64_t)cap.sock.tag);
		break;
	default:
		alt_printf("unknown  raw:%X\n", cap.raw);
		break;
	}
}

void util_print_cap_at(s3k_cidx_t idx)
{
	s3k_cap_t cap;
	s3k_err_t err = s3k_cap_read(idx, &cap);

	alt_printf("%D: ", (uint64_t)idx);
	if (err != S3K_SUCCESS) {
		alt_printf("%s\n", util_err_str(err));
		return;
	}
	util_print_cap(cap);
}

void util_dump_caps(void)
{
	s3k_cap_t cap;

	alt_printf("capability table of pid %D:\n", s3k_get_pid());
	for (s3k_cidx_t i = 0; i < S3K_CAP_CNT; i++) {
		if (s3k_cap_read(i, &cap) != S3K_SUCCESS)
			continue;
		alt_printf("%D: ", (uint64_t)i);
		util_print_cap(cap);
	}
}

/* ------------------------------------------------------------------ */
/* Capability slots                                                   */
/* ------------------------------------------------------------------ */

bool util_cap_is_free(s3k_cidx_t idx)
{
	s3k_cap_t cap;

	if (idx >= S3K_CAP_CNT)
		return false;
	/* Reading an empty slot fails with S3K_ERR_EMPTY. Any other failure
	 * means the index is unusable, which is also not free. */
	return s3k_cap_read(idx, &cap) == S3K_ERR_EMPTY;
}

s3k_cidx_t util_find_free_cap_from(s3k_cidx_t bgn)
{
	for (s3k_cidx_t i = bgn; i < S3K_CAP_CNT; i++) {
		if (util_cap_is_free(i))
			return i;
	}
	return UTIL_NO_CAP;
}

s3k_cidx_t util_find_free_cap(void)
{
	return util_find_free_cap_from(FREE_CAP_BGN);
}

/* ------------------------------------------------------------------ */
/* Device access                                                      */
/* ------------------------------------------------------------------ */

s3k_err_t util_setup_uart(s3k_cidx_t mem_idx, s3k_cidx_t dst_idx,
			  s3k_pmp_slot_t slot, s3k_addr_t base,
			  s3k_addr_t size)
{
	s3k_napot_t addr = s3k_napot_encode(base, size);
	s3k_err_t err;

	err = s3k_cap_derive(mem_idx, dst_idx, s3k_mk_pmp(addr, S3K_MEM_RW));
	if (err != S3K_SUCCESS)
		return err;

	err = s3k_pmp_load(dst_idx, slot);
	if (err != S3K_SUCCESS)
		return err;

	/* Without this the frame exists only in the shadow state and the
	 * hardware still refuses the store. */
	s3k_sync_mem();
	return S3K_SUCCESS;
}

/* ------------------------------------------------------------------ */
/* Traps                                                              */
/* ------------------------------------------------------------------ */

void util_setup_trap(void (*handler)(void), void *stack_base,
		     size_t stack_size)
{
	s3k_reg_write(S3K_REG_TPC, (uint64_t)handler);
	s3k_reg_write(S3K_REG_TSP, (uint64_t)stack_base + stack_size);
}

void util_default_trap_handler(void)
{
	/* On a trap the kernel has already done:
	 *   epc    = pc          (faulting instruction)
	 *   pc     = tpc         (this function)
	 *   esp    = sp          (stack at the fault)
	 *   sp     = tsp         (trap stack)
	 *   ecause = mcause      (RISC-V privileged spec)
	 *   eval   = mtval */
	uint64_t epc = s3k_reg_read(S3K_REG_EPC);
	uint64_t esp = s3k_reg_read(S3K_REG_ESP);
	uint64_t ecause = s3k_reg_read(S3K_REG_ECAUSE);
	uint64_t eval = s3k_reg_read(S3K_REG_EVAL);

	alt_printf("trap: epc:%X esp:%X ecause:%X eval:%X\n", epc, esp, ecause,
		   eval);

	/* Returning restores pc from epc and sp from esp, so the faulting
	 * instruction runs again. To skip it instead, advance epc by the
	 * instruction width before returning:
	 *
	 *     s3k_reg_write(S3K_REG_EPC, epc + 4);
	 *
	 * which is only correct for a 32-bit instruction; compressed
	 * instructions are 2 bytes. */
}

/* ------------------------------------------------------------------ */
/* Starting another process                                           */
/* ------------------------------------------------------------------ */

s3k_err_t util_grant_memory(s3k_pid_t pid, s3k_cidx_t mem_src, s3k_addr_t base,
			    s3k_addr_t size, s3k_rwx_t rwx, s3k_cidx_t dst_idx,
			    s3k_pmp_slot_t dst_slot, s3k_cidx_t *keep_mem)
{
	s3k_cidx_t mem = util_find_free_cap();
	s3k_cidx_t pmp;
	s3k_err_t err;

	if (mem == UTIL_NO_CAP)
		return S3K_ERR_DST_OCCUPIED;

	err = s3k_cap_derive(mem_src, mem,
			     s3k_mk_memory(base, base + size, rwx));
	if (err != S3K_SUCCESS)
		return err;

	/* The slice now occupies mem, so the next free slot is a different
	 * one. This is why util_find_free_cap must be called again rather
	 * than reused. */
	pmp = util_find_free_cap();
	if (pmp == UTIL_NO_CAP)
		return S3K_ERR_DST_OCCUPIED;

	err = s3k_cap_derive(mem, pmp,
			     s3k_mk_pmp(s3k_napot_encode(base, size), rwx));
	if (err != S3K_SUCCESS)
		return err;

	err = s3k_mon_cap_move(MONITOR, s3k_get_pid(), pmp, pid, dst_idx);
	if (err != S3K_SUCCESS)
		return err;

	err = s3k_mon_pmp_load(MONITOR, pid, dst_idx, dst_slot);
	if (err != S3K_SUCCESS)
		return err;

	if (keep_mem != NULL)
		*keep_mem = mem;
	return S3K_SUCCESS;
}

s3k_err_t util_grant_device(s3k_pid_t pid, s3k_cidx_t mem_src, s3k_addr_t base,
			    s3k_addr_t size, s3k_rwx_t rwx, s3k_cidx_t dst_idx,
			    s3k_pmp_slot_t dst_slot)
{
	s3k_cidx_t pmp = util_find_free_cap();
	s3k_err_t err;

	if (pmp == UTIL_NO_CAP)
		return S3K_ERR_DST_OCCUPIED;

	err = s3k_cap_derive(mem_src, pmp,
			     s3k_mk_pmp(s3k_napot_encode(base, size), rwx));
	if (err != S3K_SUCCESS)
		return err;

	err = s3k_mon_cap_move(MONITOR, s3k_get_pid(), pmp, pid, dst_idx);
	if (err != S3K_SUCCESS)
		return err;

	return s3k_mon_pmp_load(MONITOR, pid, dst_idx, dst_slot);
}

s3k_err_t util_start(s3k_pid_t pid, s3k_addr_t entry)
{
	s3k_err_t err = s3k_mon_reg_write(MONITOR, pid, S3K_REG_PC, entry);

	if (err != S3K_SUCCESS)
		return err;
	return s3k_mon_resume(MONITOR, pid);
}

/* ------------------------------------------------------------------ */
/* Scheduling                                                         */
/* ------------------------------------------------------------------ */

s3k_err_t util_grant_time(s3k_pid_t pid, s3k_cidx_t src, s3k_time_slot_t bgn,
			  s3k_time_slot_t end, s3k_cidx_t dst_idx)
{
	s3k_cidx_t slice = util_find_free_cap();
	s3k_cap_t cap;
	s3k_err_t err;

	if (slice == UTIL_NO_CAP)
		return S3K_ERR_DST_OCCUPIED;

	/* The hart number is a property of the source capability, so read it
	 * rather than assuming hart 0. */
	err = s3k_cap_read(src, &cap);
	if (err != S3K_SUCCESS)
		return err;
	if (cap.type != S3K_CAPTY_TIME)
		return S3K_ERR_INVALID_CAPABILITY;

	err = s3k_cap_derive(src, slice, s3k_mk_time(cap.time.hart, bgn, end));
	if (err != S3K_SUCCESS)
		return err;

	err = s3k_mon_cap_move(MONITOR, s3k_get_pid(), slice, pid, dst_idx);
	if (err != S3K_SUCCESS)
		return err;

	/* Rebuild the schedule so the new owner is admitted. */
	s3k_sync();
	return S3K_SUCCESS;
}

s3k_err_t util_move_time(s3k_pid_t pid, s3k_cidx_t src, s3k_cidx_t dst_idx)
{
	s3k_err_t err
	    = s3k_mon_cap_move(MONITOR, s3k_get_pid(), src, pid, dst_idx);

	if (err != S3K_SUCCESS)
		return err;
	s3k_sync();
	return S3K_SUCCESS;
}

/* ------------------------------------------------------------------ */
/* IPC                                                                */
/* ------------------------------------------------------------------ */

s3k_err_t util_make_socket_pair(s3k_chan_t chan, s3k_ipc_mode_t mode,
				s3k_ipc_perm_t perm, uint32_t client_tag,
				s3k_cidx_t *server, s3k_cidx_t *client)
{
	s3k_cidx_t srv = util_find_free_cap();
	s3k_cidx_t cli;
	s3k_err_t err;

	if (client_tag == 0)
		return S3K_ERR_INVALID_CAPABILITY;
	if (srv == UTIL_NO_CAP)
		return S3K_ERR_DST_OCCUPIED;

	/* Tag 0 makes this the receiving end. */
	err = s3k_cap_derive(CHANNEL, srv,
			     s3k_mk_socket(chan, mode, perm, 0));
	if (err != S3K_SUCCESS)
		return err;

	cli = util_find_free_cap();
	if (cli == UTIL_NO_CAP)
		return S3K_ERR_DST_OCCUPIED;

	/* The client is derived from the server, not from the channel. */
	err = s3k_cap_derive(srv, cli,
			     s3k_mk_socket(chan, mode, perm, client_tag));
	if (err != S3K_SUCCESS)
		return err;

	*server = srv;
	*client = cli;
	return S3K_SUCCESS;
}

s3k_reply_t util_sendrecv_retry(s3k_cidx_t sock, const s3k_msg_t *msg)
{
	s3k_reply_t reply;

	do {
		reply = s3k_sock_sendrecv(sock, msg);
	} while (reply.err == S3K_ERR_TIMEOUT
		 || reply.err == S3K_ERR_NO_RECEIVER);
	return reply;
}

void util_server_loop(s3k_cidx_t sock, util_handler_t handler, void *ctx,
		      bool recv_cap)
{
	s3k_msg_t msg = {0};
	s3k_reply_t reply;

	for (;;) {
		do {
			if (recv_cap) {
				msg.cap_idx = util_find_free_cap();
				msg.send_cap = msg.cap_idx != UTIL_NO_CAP;
			}
			reply = s3k_sock_sendrecv(sock, &msg);
		} while (reply.err != S3K_SUCCESS);
		msg = handler(reply, ctx);
	}
}

bool util_wait_blocked(s3k_pid_t pid)
{
	s3k_state_t state;

	for (;;) {
		if (s3k_mon_state_get(MONITOR, pid, &state) != S3K_SUCCESS)
			return false;
		if (state & S3K_PSF_BLOCKED)
			return true;
		/* Hand the target some of our time so it can reach the
		 * receive. Without this the loop spins until our slot ends. */
		s3k_mon_yield(MONITOR, pid);
	}
}
