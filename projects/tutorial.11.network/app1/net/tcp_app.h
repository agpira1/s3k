#pragma once

#include <stddef.h>
#include <stdint.h>

/* Application adapter used by tcp.c to avoid knowing HTTP/telnet details. */
typedef void (*tcp_app_send_fn)(const uint8_t *data, size_t len);

void tcp_app_init(tcp_app_send_fn send);
uint16_t tcp_app_port(void);
const char *tcp_app_name(void);
void tcp_app_on_connect(void);
void tcp_app_on_data(const uint8_t *data, size_t len);
void tcp_app_on_close(void);
