#include "tcp_app.h"

#include "app_protocol.h"
#include "http.h"
#include "telnet.h"


/* Initialize both apps; dispatch later chooses the active one. */
void tcp_app_init(tcp_app_send_fn send) {
    telnet_app_init(send);
}

uint16_t tcp_app_port(void) {
    /* HTTP listens on port 80, telnet on port 23. */
    return telnet_app_port();
}

const char *tcp_app_name(void) {
    return "telnet";
}

void tcp_app_on_connect(void) {
    telnet_app_on_connect();
}

void tcp_app_on_data(const uint8_t *data, size_t len) {
    /* TCP transport does not inspect app bytes; it only dispatches them. */
    telnet_app_on_data(data, len);
}

void tcp_app_on_close(void) {
    telnet_app_on_close();
}
