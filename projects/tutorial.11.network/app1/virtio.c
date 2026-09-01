#include "virtio.h"
#include "console.h"

static int g_virtio_version;

/* Remember the MMIO transport version discovered during probe. */
int virtio_get_version(void) {
    return g_virtio_version;
}

int virtio_probe_net(void) {
    /* "virt" in little endian, per the VirtIO MMIO transport. */
    if (virtio_read32(VIRTIO_MMIO_MAGIC_VALUE) != 0x74726976U) {
        console_puts("virtio: bad magic\n");
        return -1;
    }
    g_virtio_version = (int)virtio_read32(VIRTIO_MMIO_VERSION);
    console_puts("virtio: version ");
    console_puthex((unsigned long)g_virtio_version);
    console_puts(" device ");
    console_puthex((unsigned long)virtio_read32(VIRTIO_MMIO_DEVICE_ID));
    console_puts("\n");
    if (g_virtio_version != 0x1 && g_virtio_version != 0x2) {
        console_puts("virtio: unsupported version ");
        console_puthex((unsigned long)g_virtio_version);
        console_puts("\n");
        return -1;
    }
    /* This kernel only drives a virtio-net device, not block/console/etc. */
    if (virtio_read32(VIRTIO_MMIO_DEVICE_ID) != 0x1U) {
        console_puts("virtio: not a net device\n");
        return -1;
    }
    return 0;
}
