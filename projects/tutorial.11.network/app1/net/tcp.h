#pragma once

#include <stddef.h>
#include <stdint.h>

/* Minimal TCP transport API used by the IPv4 layer. */
void tcp_init(const uint8_t mac[6], const uint8_t ip[4]);
uint16_t tcp_listen_port(void);
void tcp_handle_segment(const uint8_t *tcp_bytes, size_t len, const uint8_t src_ip[4], const uint8_t src_mac[6]);
