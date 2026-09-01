#pragma once

#include <stddef.h>

/* VirtIO-net driver entry points used by main and the IP stack. */
int virtio_net_init();
void virtio_net_poll(void);
int virtio_net_send(const void *data, size_t len);
