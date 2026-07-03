#pragma once

/*
 * Boot memory capability placement ordinals.
 *
 * Concrete capability indices are derived from these ordinals and the
 * configured MAX_MEMORY_FUEL block size.
 */
#define BOOT_MEM_RAM_ORD 0
#define BOOT_MEM_UART_ORD 1
#define BOOT_MEM_SPM_ORD 2

#define BOOT_MEM_RAM_IDX (BOOT_MEM_RAM_ORD * MAX_MEMORY_FUEL)
#define BOOT_MEM_UART_IDX (BOOT_MEM_UART_ORD * MAX_MEMORY_FUEL)
#define BOOT_MEM_SPM_IDX (BOOT_MEM_SPM_ORD * MAX_MEMORY_FUEL)

#define S3K_BOOT_MEM_RAM_ORD 0
#define S3K_BOOT_MEM_UART_ORD 1
#define S3K_BOOT_MEM_SPM_ORD 2

#define S3K_BOOT_MEM_RAM_IDX BOOT_MEM_RAM_IDX
#define S3K_BOOT_MEM_UART_IDX BOOT_MEM_UART_IDX
#define S3K_BOOT_MEM_SPM_IDX BOOT_MEM_SPM_IDX
