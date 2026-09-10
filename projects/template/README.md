# Project template

Created by `make new NAME=<project-name> [APPS=<n>]` from the repository root.
`APPS` is the highest application index: `0`, the default, creates `app0`
alone, and `2` creates `app0`, `app1`, and `app2`.

## Files

| File | Purpose |
| --- | --- |
| `Makefile` | Build rules. Edit `APPS` and the default `PLATFORM`. |
| `s3k_conf.h` | Process, capability, channel, and schedule counts. Force-included into both the kernel and every application in this project. |
| `default.ld` | Shared section layout. Do not edit. |
| `app0.ld` | Physical region for `app0`. Do not change `ORIGIN`. |
| `app0/main.c` | Application entry point. |
| `utils.h` / `utils.c` | Helpers over the kernel API. Not kernel API themselves. Compiled into every application via `SHARED_SRCS` in the `Makefile`. |

## Build and run

```sh
make -C projects/<name>            # build kernel and apps
make -C projects/<name> qemu       # run, exit with Ctrl-a then x
make -C projects/<name> size       # image sizes
make -C projects/<name> clean
```

Debugging takes two terminals:

```sh
make -C projects/<name> qemu-gdb   # terminal 1: start paused, GDB on :3333
make -C projects/<name> gdb        # terminal 2: attach with all symbols
```

## Why `app0.ld` says `0x80010000`

The kernel occupies `0x80000000` plus 64 KiB, and its linker script defines
`_payload` at the end of that region. `proc_init` sets process 0's program
counter to `_payload` and loads initial capability 0, a PMP frame covering
`0x80010000` + 64 KiB. Three numbers must agree — the kernel's `_payload`, this
linker script's `ORIGIN`, and `INIT_CAPS[0]` — or the system faults before the
first instruction runs.

## `utils.h`

Thin wrappers over `common/inc/s3k/`, plus the boot capability index names.
Nothing here is kernel API; every `util_` function is a few `s3k_` calls with
the error checking the tutorials leave out.

| Function | Does |
| --- | --- |
| `util_err_str` | Error code to name, e.g. `S3K_ERR_INVALID_DERIVATION`. |
| `util_check` | Report a failed call; silent on success. Returns `true` when it succeeded. |
| `util_print_cap`, `util_print_cap_at`, `util_dump_caps` | Readable capability dumps. |
| `util_cap_is_free`, `util_find_free_cap`, `util_find_free_cap_from` | Find an empty slot. Returns `UTIL_NO_CAP`, never `0`, when the table is full. |
| `util_setup_uart` | Derive, load, and sync a PMP frame over a device range. |
| `util_setup_trap`, `util_default_trap_handler` | Install a handler and its stack. |
| `util_grant_memory`, `util_grant_device`, `util_start` | Hand another process memory, a device, and an entry point. `util_start` also resumes it. |
| `util_grant_time`, `util_move_time` | Give another process a time slice or a whole hart. |
| `util_make_socket_pair`, `util_sendrecv_retry`, `util_server_loop`, `util_wait_blocked` | IPC. |

These differ from `projects/tutorial-commons/utils.h` in four ways. That file
is a header full of non-`static` definitions included by relative path, so a
second translation unit including it breaks the link; this one is a proper
header plus one `.c`. `find_free_cap` there returns `0` on exhaustion, which is
a valid index — `BOOT_PMP`, the frame this code executes from. Its
`s3k_print_cap` uses `%Z`, which `alt_printf` does not implement, so the time
range end and the PMP address print as nothing. And its `log_sys` is declared
`uint32_t` with no `return` statement.

To share these with a second application, nothing is needed: `SHARED_SRCS` in
the `Makefile` already compiles `utils.c` into every entry in `APPS`.

## Adding a second process

1. `s3k_conf.h`: confirm `S3K_PROC_CNT` is at least 2.
2. `Makefile`: `APPS=app0 app1`, and set `PLATFORM ?=qemu_virt4` if the second
   process should run on its own hart.
3. Copy `app0.ld` to `app1.ld` and change `ORIGIN` to `0x80020000`. Regions must
   not overlap, must sit inside the `RAM_MEM` capability's range, and should be
   naturally aligned powers of two so a single PMP frame can cover them.
4. Create `app1/main.c`.
5. In `app0/main.c`, before the process can run, use the monitor capability to
   give it what it needs:

```c
#define APP1_PID 1
#define APP1_BASE 0x80020000
#define APP1_SIZE 0x10000

/* Slots in app1's table, and app1's hardware PMP slots. Both start empty. */
#define APP1_MEM_CAP 0
#define APP1_UART_CAP 1
#define APP1_TIME_CAP 2
#define APP1_MEM_SLOT 0
#define APP1_UART_SLOT 1

s3k_cidx_t app1_mem; /* the deed we keep, so we can revoke later */

util_check("app1 memory",
           util_grant_memory(APP1_PID, RAM_MEM, APP1_BASE, APP1_SIZE,
                             S3K_MEM_RWX, APP1_MEM_CAP, APP1_MEM_SLOT,
                             &app1_mem));
util_check("app1 uart",
           util_grant_device(APP1_PID, UART_MEM, UART0_BASE_ADDR, 0x8,
                             S3K_MEM_RW, APP1_UART_CAP, APP1_UART_SLOT));
util_check("app1 time",
           util_move_time(APP1_PID, HART1_TIME, APP1_TIME_CAP));
util_check("app1 start", util_start(APP1_PID, APP1_BASE));
```

Three things are easy to get wrong here.

The memory capability stays in *this* process's table, at `app1_mem`. That is
deliberate: the parent keeps the deed so it can revoke the grant, and the child
receives only the gate key. `util_grant_device` has no equivalent, because it
derives from a device capability the parent already holds.

`util_start` resumes as well as setting the program counter. `proc_init` in
`kernel/src/proc.c` leaves every process except process 0 suspended, and a
suspended process is skipped by the scheduler even when it holds time. Setting
the program counter alone starts nothing.

The PC must equal `app1.ld`'s `ORIGIN`. There is no loader — QEMU places each
ELF at its linked address, and setting the program counter is the whole act of
starting a program.

## Reference

Full API and design documentation: <https://marcusson.dev/s3k-kernel>
