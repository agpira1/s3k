#include "altc/altio.h"
#include "s3k/s3k.h"
#include "utils.h"

/* Free slots in this process's own capability table run from FREE_CAP_BGN to
 * S3K_CAP_CNT - 1. Name every slot you use; a bare number in a derive call is
 * how the tutorials become unreadable. */
#define UART_CAP FREE_CAP_BGN /* 10 */

/* Hardware PMP slots run from 0 to S3K_PMP_CNT - 1. This is a different
 * namespace from capability indices: slot 0 already holds BOOT_PMP, the frame
 * covering this program's own memory, which is why code can execute at all. */
#define UART_PMP_SLOT 1

/* The trap handler runs on its own stack, because the fault may well have been
 * caused by the ordinary stack. Static storage: the kernel keeps the pointer
 * across the whole life of the process. */
static char trap_stack[1024];

int main(void)
{
	s3k_err_t err;

	/* Until this succeeds there is nowhere to print, so a failure here is
	 * necessarily silent. Inspect it under GDB:
	 *   make -C projects/<name> qemu-gdb   and   make -C projects/<name> gdb
	 *
	 * qemu_virt exposes 0x1000 bytes at UART0_BASE_ADDR and qemu_virt4
	 * exposes 0x2000; 0x8 is enough for the NS16550A registers themselves
	 * and is what a program needs unless it also talks to virtio. */
	err = util_setup_uart(UART_MEM, UART_CAP, UART_PMP_SLOT,
			      UART0_BASE_ADDR, 0x8);
	if (err != S3K_SUCCESS)
		return 1;

	/* From here on failures can be reported. */
	util_setup_trap(util_default_trap_handler, trap_stack,
			sizeof trap_stack);

	/* alt_printf is not C printf. It supports %c %s %x %X %d %D and %%
	 * only, with no width or precision. Use the capital forms for 64-bit
	 * values; %x truncates to 32 bits. */
	alt_printf("hello from pid %X at tick %X\n", s3k_get_pid(),
		   s3k_get_time());

	util_dump_caps();

	/* Returning from main lands in an infinite loop in start.S. There is no
	 * exit; a finished process simply idles in the slots it owns. */
	while (1)
		;
}
