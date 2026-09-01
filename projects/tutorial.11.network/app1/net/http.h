#pragma once

#include "tcp_app.h"

/* HTTP app: GET /?num=x returns x+1 in a text/plain response. */
void http_app_init(tcp_app_send_fn send);
uint16_t http_app_port(void);
void http_app_on_connect(void);
void http_app_on_data(const uint8_t *data, size_t len);
void http_app_on_close(void);
