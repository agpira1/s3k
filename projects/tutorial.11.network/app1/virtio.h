#pragma once

#include <stdint.h>

#define VIRTIO_MMIO_BASE 0x10001000UL

/* Register offsets for the VirtIO MMIO transport used by QEMU virt. */
enum {
    VIRTIO_MMIO_MAGIC_VALUE = 0x000,
    VIRTIO_MMIO_VERSION = 0x004,
    VIRTIO_MMIO_DEVICE_ID = 0x008,
    VIRTIO_MMIO_VENDOR_ID = 0x00c,
    VIRTIO_MMIO_DEVICE_FEATURES = 0x010,
    VIRTIO_MMIO_DEVICE_FEATURES_SEL = 0x014,
    VIRTIO_MMIO_DRIVER_FEATURES = 0x020,
    VIRTIO_MMIO_DRIVER_FEATURES_SEL = 0x024,
    VIRTIO_MMIO_GUEST_PAGE_SIZE = 0x028,
    VIRTIO_MMIO_QUEUE_SEL = 0x030,
    VIRTIO_MMIO_QUEUE_NUM_MAX = 0x034,
    VIRTIO_MMIO_QUEUE_NUM = 0x038,
    VIRTIO_MMIO_QUEUE_ALIGN = 0x03c,
    VIRTIO_MMIO_QUEUE_PFN = 0x040,
    VIRTIO_MMIO_QUEUE_READY = 0x044,
    VIRTIO_MMIO_QUEUE_NOTIFY = 0x050,
    VIRTIO_MMIO_INTERRUPT_STATUS = 0x060,
    VIRTIO_MMIO_INTERRUPT_ACK = 0x064,
    VIRTIO_MMIO_STATUS = 0x070,
    VIRTIO_MMIO_QUEUE_DESC_LOW = 0x080,
    VIRTIO_MMIO_QUEUE_DESC_HIGH = 0x084,
    VIRTIO_MMIO_QUEUE_DRIVER_LOW = 0x090,
    VIRTIO_MMIO_QUEUE_DRIVER_HIGH = 0x094,
    VIRTIO_MMIO_QUEUE_DEVICE_LOW = 0x0a0,
    VIRTIO_MMIO_QUEUE_DEVICE_HIGH = 0x0a4,
    VIRTIO_MMIO_CONFIG_GENERATION = 0x0fc,
    VIRTIO_MMIO_CONFIG = 0x100
};

/* Device status bits written during VirtIO feature negotiation. */
enum {
    VIRTIO_STATUS_ACKNOWLEDGE = 1,
    VIRTIO_STATUS_DRIVER = 2,
    VIRTIO_STATUS_DRIVER_OK = 4,
    VIRTIO_STATUS_FEATURES_OK = 8,
    VIRTIO_STATUS_FAILED = 0x80
};

#define VIRTIO_F_VERSION_1 (1ULL << 32)
#define VIRTIO_NET_F_MAC (1ULL << 5)

/* Raw MMIO helpers keep volatile access localized and readable. */
static inline volatile uint32_t *virtio_reg32(uint32_t offset) {
    return (volatile uint32_t *)(VIRTIO_MMIO_BASE + offset);
}

static inline volatile uint8_t *virtio_reg8(uint32_t offset) {
    return (volatile uint8_t *)(VIRTIO_MMIO_BASE + offset);
}

static inline uint32_t virtio_read32(uint32_t offset) {
    return *virtio_reg32(offset);
}

static inline void virtio_write32(uint32_t offset, uint32_t value) {
    *virtio_reg32(offset) = value;
}

static inline uint8_t virtio_read8(uint32_t offset) {
    return *virtio_reg8(offset);
}

static inline void virtio_mb(void) {
    asm volatile("fence iorw, iorw" ::: "memory");
}

int virtio_probe_net(void);
int virtio_get_version(void);
