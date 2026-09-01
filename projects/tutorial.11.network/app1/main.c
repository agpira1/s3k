#include "s3k.h"
#include <stdio.h>
#include "console.h"
#include "virtio_net.h"

extern char __uart_base[]; // UART base address
extern char __virtio_base[]; // VIRTIO base address

// Enable memory at mem capability idx
void setup_dev_mem(int cap_idx, s3k_word_t base, s3k_word_t size, s3k_word_t slot) {
	s3k_word_t perm = S3K_MEM_PERM_RW; // Read/Write permissions
	s3k_word_t addr = s3k_pmp_napot_encode(base, size);
	s3k_mem_pmp_set(cap_idx, slot, perm, addr);
    return;
}
void setup_uart(int cap_idx) {
	setup_dev_mem(cap_idx, (s3k_word_t)__uart_base, 0x20, 2);
}
void setup_virtio(int cap_idx) {
	setup_dev_mem(cap_idx, (s3k_word_t)__virtio_base, 0x1000, 3);
}

/* UART banner gives a fast "did the CPU reach C?" signal during bring-up. */
static void banner(void) {
    console_puts("\n");
    console_puts("========================================\n");
    console_puts("  Bare-metal RISC-V network starter\n");
    console_puts("========================================\n");
}


int main(void)
{
	setup_uart(S3K_BOOT_MEM_UART_IDX);
	setup_virtio(S3K_BOOT_MEM_VIRTIO_IDX);
	s3k_sync();

	banner();

    /* VirtIO-net owns the lower device path; the app protocol is chosen here. */
    if (virtio_net_init() == 0) {
		console_puts("VirtIO-net ready; telnet on port 23\n");
    } else {
        console_puts("VirtIO-net init failed\n");
    }

    for (;;) {
        virtio_net_poll();
    }
}
