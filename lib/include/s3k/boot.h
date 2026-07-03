#pragma once

/*
 * Boot memory capability placement ordinals.
 *
 * Concrete capability indices are derived from these ordinals and the
 * configured MAX_MEMORY_FUEL block size.
 */
#define S3K_BOOT_MEM_RAM_ORD 0
#define S3K_BOOT_MEM_UART_ORD 1
#define S3K_BOOT_MEM_SPM_ORD 2

#define S3K_BOOT_MEM_RAM_IDX (S3K_BOOT_MEM_RAM_ORD * MAX_MEMORY_FUEL)
#define S3K_BOOT_MEM_UART_IDX (S3K_BOOT_MEM_UART_ORD * MAX_MEMORY_FUEL)
#define S3K_BOOT_MEM_SPM_IDX (S3K_BOOT_MEM_SPM_ORD * MAX_MEMORY_FUEL)
