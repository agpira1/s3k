#include "virtio_net.h"

#include "console.h"
#include "net/net.h"
#include "string.h"
#include "virtio.h"

#include <stdint.h>

#define RX_QUEUE_SIZE 8U
#define TX_QUEUE_SIZE 1U
#define MAX_PACKET_SIZE 2048U
#define VIRTIO_NET_HDR_SIZE 12U
#define QUEUE_ALIGN 4096U

#define VIRTQ_DESC_F_NEXT 1U
#define VIRTQ_DESC_F_WRITE 2U

/* Virtqueue descriptor shared with the device. */
typedef struct {
    uint64_t addr;
    uint32_t len;
    uint16_t flags;
    uint16_t next;
} __attribute__((packed)) vring_desc_t;

/* RX and TX rings have different sizes, so their packed ring structs differ. */
typedef struct {
    uint16_t flags;
    uint16_t idx;
    uint16_t ring[RX_QUEUE_SIZE];
    uint16_t used_event;
} __attribute__((packed)) vring_avail_rx_t;

typedef struct {
    uint16_t flags;
    uint16_t idx;
    struct {
        uint32_t id;
        uint32_t len;
    } ring[RX_QUEUE_SIZE];
    uint16_t avail_event;
} __attribute__((packed)) vring_used_rx_t;

typedef struct {
    uint16_t flags;
    uint16_t idx;
    uint16_t ring[TX_QUEUE_SIZE];
    uint16_t used_event;
} __attribute__((packed)) vring_avail_tx_t;

typedef struct {
    uint16_t flags;
    uint16_t idx;
    struct {
        uint32_t id;
        uint32_t len;
    } ring[TX_QUEUE_SIZE];
    uint16_t avail_event;
} __attribute__((packed)) vring_used_tx_t;

typedef struct {
    uint8_t flags;
    uint8_t gso_type;
    uint16_t hdr_len;
    uint16_t gso_size;
    uint16_t csum_start;
    uint16_t csum_offset;
    uint16_t num_buffers;
} __attribute__((packed)) virtio_net_hdr_t;

static uint8_t rx_mem[QUEUE_ALIGN * 2U] __attribute__((aligned(QUEUE_ALIGN)));
static uint8_t tx_mem[QUEUE_ALIGN * 2U] __attribute__((aligned(QUEUE_ALIGN)));
static uint8_t rx_buffers[RX_QUEUE_SIZE][VIRTIO_NET_HDR_SIZE + MAX_PACKET_SIZE] __attribute__((aligned(16)));
static uint8_t tx_buffer[VIRTIO_NET_HDR_SIZE + MAX_PACKET_SIZE] __attribute__((aligned(16)));

static vring_desc_t *rx_desc;
static vring_avail_rx_t *rx_avail;
static vring_used_rx_t *rx_used;
static vring_desc_t *tx_desc;
static vring_avail_tx_t *tx_avail;
static vring_used_tx_t *tx_used;
static uint16_t rx_avail_idx;
static uint16_t rx_used_idx;
static uint16_t tx_avail_idx;
static uint16_t tx_used_idx;

static uint8_t g_mac[6];
static int g_ready;

/* Virtqueue used ring must start on the transport-required alignment. */
static size_t align_up(size_t value, size_t align) {
    return (value + align - 1U) & ~(align - 1U);
}

static uint64_t read_device_features(void) {
    /* Features are split across two 32-bit selector windows. */
    virtio_write32(VIRTIO_MMIO_DEVICE_FEATURES_SEL, 0);
    uint64_t low = virtio_read32(VIRTIO_MMIO_DEVICE_FEATURES);
    virtio_write32(VIRTIO_MMIO_DEVICE_FEATURES_SEL, 1);
    uint64_t high = virtio_read32(VIRTIO_MMIO_DEVICE_FEATURES);
    return low | (high << 32);
}

static void write_driver_features(uint64_t features) {
    virtio_write32(VIRTIO_MMIO_DRIVER_FEATURES_SEL, 0);
    virtio_write32(VIRTIO_MMIO_DRIVER_FEATURES, (uint32_t)(features & 0xffffffffULL));
    virtio_write32(VIRTIO_MMIO_DRIVER_FEATURES_SEL, 1);
    virtio_write32(VIRTIO_MMIO_DRIVER_FEATURES, (uint32_t)(features >> 32));
}

static int setup_queue(int version, uint16_t queue_index, uint16_t queue_size, uint8_t *mem, size_t mem_size, void **desc_out, void **avail_out, void **used_out) {
    size_t desc_bytes = (size_t)queue_size * sizeof(vring_desc_t);
    size_t avail_bytes;
    size_t used_offset;

    virtio_write32(VIRTIO_MMIO_QUEUE_SEL, queue_index);
    if (virtio_read32(VIRTIO_MMIO_QUEUE_NUM_MAX) < queue_size) {
        return -1;
    }

    if (queue_size == RX_QUEUE_SIZE) {
        avail_bytes = sizeof(vring_avail_rx_t);
    } else {
        avail_bytes = sizeof(vring_avail_tx_t);
    }

    /* Layout is descriptor table, available ring, padding, then used ring. */
    used_offset = align_up(desc_bytes + avail_bytes, QUEUE_ALIGN);
    if (used_offset + (queue_size == RX_QUEUE_SIZE ? sizeof(vring_used_rx_t) : sizeof(vring_used_tx_t)) > mem_size) {
        return -1;
    }

    memset(mem, 0, mem_size);
    *desc_out = mem;
    *avail_out = mem + desc_bytes;
    *used_out = mem + used_offset;

    virtio_write32(VIRTIO_MMIO_QUEUE_NUM, queue_size);
    if (version == 0x2) {
        /* Modern MMIO transport takes explicit guest physical addresses. */
        uintptr_t desc_addr = (uintptr_t)mem;
        uintptr_t driver_addr = (uintptr_t)mem + desc_bytes;
        uintptr_t device_addr = (uintptr_t)mem + used_offset;
        virtio_write32(VIRTIO_MMIO_QUEUE_DESC_LOW, (uint32_t)desc_addr);
        virtio_write32(VIRTIO_MMIO_QUEUE_DESC_HIGH, (uint32_t)(desc_addr >> 32));
        virtio_write32(VIRTIO_MMIO_QUEUE_DRIVER_LOW, (uint32_t)driver_addr);
        virtio_write32(VIRTIO_MMIO_QUEUE_DRIVER_HIGH, (uint32_t)(driver_addr >> 32));
        virtio_write32(VIRTIO_MMIO_QUEUE_DEVICE_LOW, (uint32_t)device_addr);
        virtio_write32(VIRTIO_MMIO_QUEUE_DEVICE_HIGH, (uint32_t)(device_addr >> 32));
        virtio_write32(VIRTIO_MMIO_QUEUE_READY, 1);
        return virtio_read32(VIRTIO_MMIO_QUEUE_READY) == 1U ? 0 : -1;
    }

    /* Legacy MMIO transport uses a page-frame number instead. */
    virtio_write32(VIRTIO_MMIO_GUEST_PAGE_SIZE, QUEUE_ALIGN);
    virtio_write32(VIRTIO_MMIO_QUEUE_ALIGN, QUEUE_ALIGN);
    virtio_write32(VIRTIO_MMIO_QUEUE_PFN, (uint32_t)((uintptr_t)mem >> 12));
    virtio_write32(VIRTIO_MMIO_QUEUE_READY, 1);

    return virtio_read32(VIRTIO_MMIO_QUEUE_READY) == 1U ? 0 : -1;
}

static void post_rx_buffers(void) {
    /* Hand every RX buffer to the device before the driver becomes ready. */
    for (uint16_t i = 0; i < RX_QUEUE_SIZE; ++i) {
        rx_desc[i].addr = (uint64_t)(uintptr_t)rx_buffers[i];
        rx_desc[i].len = sizeof(rx_buffers[i]);
        rx_desc[i].flags = VIRTQ_DESC_F_WRITE;
        rx_desc[i].next = 0;
        rx_avail->ring[i] = i;
    }

    rx_avail->flags = 0;
    rx_avail_idx = RX_QUEUE_SIZE;
    virtio_mb();
    rx_avail->idx = rx_avail_idx;
    virtio_write32(VIRTIO_MMIO_QUEUE_NOTIFY, 0);
}

static void rx_repost(uint16_t slot) {
    rx_avail->ring[rx_avail_idx % RX_QUEUE_SIZE] = slot;
    virtio_mb();
    rx_avail->idx = ++rx_avail_idx;
    virtio_write32(VIRTIO_MMIO_QUEUE_NOTIFY, 0);
}

static void rx_poll(void) {
    /* Polling keeps the first networking milestone independent of interrupts. */
    while (rx_used_idx != rx_used->idx) {
        uint16_t ring_index = (uint16_t)(rx_used_idx % RX_QUEUE_SIZE);
        uint32_t slot = rx_used->ring[ring_index].id;
        uint32_t len = rx_used->ring[ring_index].len;

        if (slot < RX_QUEUE_SIZE && len > VIRTIO_NET_HDR_SIZE) {
            net_handle_frame(rx_buffers[slot] + VIRTIO_NET_HDR_SIZE, len - VIRTIO_NET_HDR_SIZE);
        }

        rx_used_idx++;
        rx_repost((uint16_t)slot);
    }
}

static void tx_wait_complete(void) {
    while (tx_used_idx == tx_used->idx) {
        asm volatile("nop");
    }
    tx_used_idx++;
}

int virtio_net_send(const void *data, size_t len) {
    if (!g_ready || len > MAX_PACKET_SIZE) {
        return -1;
    }

    /* Each frame is prefixed with the VirtIO-net header before Ethernet bytes. */
    virtio_net_hdr_t *hdr = (virtio_net_hdr_t *)tx_buffer;
    uint8_t *payload = tx_buffer + VIRTIO_NET_HDR_SIZE;

    memset(hdr, 0, sizeof(*hdr));
    memcpy(payload, data, len);

    tx_desc[0].addr = (uint64_t)(uintptr_t)tx_buffer;
    tx_desc[0].len = (uint32_t)(VIRTIO_NET_HDR_SIZE + len);
    tx_desc[0].flags = 0;
    tx_desc[0].next = 0;

    tx_avail->ring[0] = 0;
    virtio_mb();
    tx_avail->idx = ++tx_avail_idx;
    virtio_write32(VIRTIO_MMIO_QUEUE_NOTIFY, 1);

    tx_wait_complete();
    return 0;
}

int virtio_net_init() {
    uint64_t features;
    uint32_t status;

    /* Probe, negotiate features, create RX/TX queues, then hand off to IP. */
    if (virtio_probe_net() != 0) {
        return -1;
    }

    int version = virtio_get_version();

    virtio_write32(VIRTIO_MMIO_STATUS, 0);
    virtio_write32(VIRTIO_MMIO_STATUS, VIRTIO_STATUS_ACKNOWLEDGE);
    virtio_write32(VIRTIO_MMIO_STATUS, VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER);

    features = read_device_features();
    if (version == 0x2) {
        write_driver_features(features & (VIRTIO_F_VERSION_1 | VIRTIO_NET_F_MAC));
    } else {
        write_driver_features(features & VIRTIO_NET_F_MAC);
    }
    virtio_write32(VIRTIO_MMIO_STATUS, VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER | VIRTIO_STATUS_FEATURES_OK);
    status = virtio_read32(VIRTIO_MMIO_STATUS);
    if ((status & VIRTIO_STATUS_FEATURES_OK) == 0) {
        console_puts("virtio-net: features rejected\n");
        virtio_write32(VIRTIO_MMIO_STATUS, VIRTIO_STATUS_FAILED);
        return -1;
    }

    for (size_t i = 0; i < sizeof(g_mac); ++i) {
        g_mac[i] = virtio_read8(VIRTIO_MMIO_CONFIG + (uint32_t)i);
    }

    if (setup_queue(version, 0, RX_QUEUE_SIZE, rx_mem, sizeof(rx_mem), (void **)&rx_desc, (void **)&rx_avail, (void **)&rx_used) != 0) {
        console_puts("virtio-net: rx queue setup failed\n");
        virtio_write32(VIRTIO_MMIO_STATUS, VIRTIO_STATUS_FAILED);
        return -1;
    }

    if (setup_queue(version, 1, TX_QUEUE_SIZE, tx_mem, sizeof(tx_mem), (void **)&tx_desc, (void **)&tx_avail, (void **)&tx_used) != 0) {
        console_puts("virtio-net: tx queue setup failed\n");
        virtio_write32(VIRTIO_MMIO_STATUS, VIRTIO_STATUS_FAILED);
        return -1;
    }

    post_rx_buffers();

    virtio_write32(VIRTIO_MMIO_STATUS, VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER | VIRTIO_STATUS_FEATURES_OK | VIRTIO_STATUS_DRIVER_OK);
    g_ready = 1;

    console_puts("virtio-net: ready\n");
    net_init(g_mac);
    return 0;
}

void virtio_net_poll(void) {
    if (!g_ready) {
        return;
    }

    rx_poll();
    asm volatile("nop");
}
