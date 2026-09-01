#include "telnet.h"

#include "console.h"
#include "int_service.h"
#include "string.h"

#define TELNET_IAC 255U
#define TELNET_DONT 254U
#define TELNET_DO 253U
#define TELNET_WONT 252U
#define TELNET_WILL 251U
#define TELNET_SB 250U
#define TELNET_SE 240U

static tcp_app_send_fn g_send;
static char g_line[128];
static size_t g_line_len;
static uint8_t g_state;
static uint8_t g_cmd;

/* Telnet output is just application data sent through the TCP adapter. */
static void send_text(const char *text) {
    g_send((const uint8_t *)text, strlen(text));
}

static void send_telnet_reply(uint8_t cmd, uint8_t option) {
    /* Refuse negotiated options; raw line input is all this server needs. */
    uint8_t reply[3] = {TELNET_IAC, 0, option};

    if (cmd == TELNET_DO) {
        reply[1] = TELNET_WONT;
    } else if (cmd == TELNET_WILL) {
        reply[1] = TELNET_DONT;
    } else {
        return;
    }

    g_send(reply, sizeof(reply));
}

static void handle_line(void) {
    /* A newline commits the current integer and returns integer+1. */
    char out[64];
    long long value;

    g_line[g_line_len] = '\0';
    if (int_service_parse(g_line, &value) == 0) {
        int_service_format(value + 1, out, sizeof(out));
        console_puts("telnet: ");
        console_puts(g_line);
        console_puts(" -> ");
        console_puts(out);
        console_puts("\n");
        send_text(out);
        send_text("\n");
    } else {
        console_puts("telnet: invalid integer\n");
        send_text("ERR\n");
    }

    g_line_len = 0;
}

static void stream_byte(uint8_t byte) {
    /* Small telnet state machine that strips command negotiation bytes. */
    if (g_state == 0) {
        if (byte == TELNET_IAC) {
            g_state = 1;
            return;
        }
        if (byte == '\r') {
            return;
        }
        if (byte == '\n') {
            handle_line();
            return;
        }
        if (g_line_len + 1 < sizeof(g_line)) {
            g_line[g_line_len++] = (char)byte;
        }
        return;
    }

    if (g_state == 1) {
        if (byte == TELNET_IAC) {
            g_state = 0;
            if (g_line_len + 1 < sizeof(g_line)) {
                g_line[g_line_len++] = (char)TELNET_IAC;
            }
            return;
        }
        if (byte == TELNET_WILL || byte == TELNET_WONT || byte == TELNET_DO || byte == TELNET_DONT) {
            g_cmd = byte;
            g_state = 2;
            return;
        }
        if (byte == TELNET_SB) {
            g_state = 3;
            return;
        }
        g_state = 0;
        return;
    }

    if (g_state == 2) {
        send_telnet_reply(g_cmd, byte);
        g_state = 0;
        return;
    }

    if (g_state == 3) {
        if (byte == TELNET_IAC) {
            g_state = 4;
        }
        return;
    }

    if (g_state == 4) {
        if (byte == TELNET_SE) {
            g_state = 0;
        } else if (byte != TELNET_IAC) {
            g_state = 3;
        }
    }
}

void telnet_app_init(tcp_app_send_fn send) {
    g_send = send;
}

uint16_t telnet_app_port(void) {
    return 23;
}

void telnet_app_on_connect(void) {
    /* New TCP connection, new line buffer. */
    g_line_len = 0;
    g_state = 0;
    g_cmd = 0;
}

void telnet_app_on_data(const uint8_t *data, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        stream_byte(data[i]);
    }
}

void telnet_app_on_close(void) {
    g_line_len = 0;
    g_state = 0;
}
