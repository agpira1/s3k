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

#define S3K_BOOT_MON_P0_ORD 0
#define S3K_BOOT_MON_P1_ORD 1

#define S3K_BOOT_MON_P0_IDX (S3K_BOOT_MON_P0_ORD * MAX_MONITOR_FUEL)
#define S3K_BOOT_MEM_P1_IDX (S3K_BOOT_MON_P1_ORD * MAX_MONITOR_FUEL)
