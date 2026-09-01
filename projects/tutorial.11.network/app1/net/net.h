#pragma once

#include <stddef.h>
#include <stdint.h>

/* Ethernet/IP dispatcher called by the VirtIO-net RX path. */
void net_init(const uint8_t mac[6]);
void net_handle_frame(const uint8_t *frame, size_t len);
