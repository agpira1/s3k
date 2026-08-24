#pragma once

/*
 * Boot memory capability placement ordinals.
 *
 * Concrete capability indices are derived from these ordinals and the
 * configured MAX_MEMORY_FUEL block size.
 */
// TODO possibly use function macros
//
#define S3K_BOOT_MEM_RAM_ORD 0
#define S3K_BOOT_MEM_UART_ORD 1
#define S3K_BOOT_MEM_SPM_ORD 2
#define S3K_BOOT_MEM_VIRTIO_ORD 2

#define S3K_BOOT_MEM_RAM_IDX (S3K_BOOT_MEM_RAM_ORD * MAX_MEMORY_FUEL)
#define S3K_BOOT_MEM_UART_IDX (S3K_BOOT_MEM_UART_ORD * MAX_MEMORY_FUEL)
#define S3K_BOOT_MEM_SPM_IDX (S3K_BOOT_MEM_SPM_ORD * MAX_MEMORY_FUEL)
#define S3K_BOOT_MEM_VIRTIO_IDX (S3K_BOOT_MEM_VIRTIO_ORD * MAX_MEMORY_FUEL)

#define S3K_BOOT_MON_P1_ORD 0
#define S3K_BOOT_MON_P2_ORD 1
#define S3K_BOOT_MON_P3_ORD 2

#define S3K_BOOT_MON_P1_IDX (S3K_BOOT_MON_P1_ORD * MAX_MONITOR_FUEL)
#define S3K_BOOT_MON_P2_IDX (S3K_BOOT_MON_P2_ORD * MAX_MONITOR_FUEL)
#define S3K_BOOT_MON_P3_IDX (S3K_BOOT_MON_P3_ORD * MAX_MONITOR_FUEL)

#define S3K_BOOT_TSL_C0_IDX (0 * MAX_TIME_FUEL)
#define S3K_BOOT_TSL_C1_IDX (1 * MAX_TIME_FUEL)

#define S3K_BOOT_IPC_0 0
