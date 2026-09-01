#include "s3k.h"
#include <stdio.h>

#include "ff.h"

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

int main(void)
{
	setup_uart(S3K_BOOT_MEM_UART_IDX);
	setup_virtio(S3K_BOOT_MEM_VIRTIO_IDX);
	s3k_sync();

	printf("hello, world\n");

	FATFS FatFs;					/* FatFs work area needed for each volume */
	f_mount(&FatFs, "", 0);			/* Give a work area to the default drive */
	printf("File system mounted\n");

	UINT bw;
	FRESULT fr;
	FIL Fil;									/* File object needed for each open file */
	fr = f_open(&Fil, "newfile.txt", FA_WRITE | FA_CREATE_ALWAYS);	/* Create a file */
	if (fr == FR_OK) {
		f_write(&Fil, "It works!\r\n", 11, &bw);	/* Write data to the file */
		fr = f_close(&Fil);							/* Close the file */
		if (fr == FR_OK && bw == 11) {
			printf("File saved\n");
		}
	} else{
		printf("File not opened\n");
	}

	char buffer[1024];
	fr = f_open(&Fil, "test.txt", FA_READ);
	if (fr == FR_OK) {
		f_read(&Fil, buffer, 1023, &bw);	/*Read data from the file */
		fr = f_close(&Fil);							/* Close the file */
		if (fr == FR_OK) {
			buffer[bw] = '\0';
			printf(buffer);
		}
	} else{
		puts("File not opened\n");
	}
}
