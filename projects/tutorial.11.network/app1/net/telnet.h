#pragma once

#include "tcp_app.h"

/* Telnet app: one integer per line, response is integer+1. */
void telnet_app_init(tcp_app_send_fn send);
uint16_t telnet_app_port(void);
void telnet_app_on_connect(void);
void telnet_app_on_data(const uint8_t *data, size_t len);
void telnet_app_on_close(void);
