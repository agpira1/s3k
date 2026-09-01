#include "http.h"

#include "console.h"
#include "int_service.h"
#include "string.h"

static tcp_app_send_fn g_send;
static char g_request[512];
static size_t g_request_len;

static int starts_with(const char *s, const char *prefix) {
    while (*prefix) {
        if (*s++ != *prefix++) {
            return 0;
        }
    }
    return 1;
}

static int find_num_param(const char *request, char *num, size_t num_len) {
    if (!starts_with(request, "GET /?")) {
        return -1;
    }

    const char *p = request + 6;
    while (*p && *p != ' ') {
        if (p[0] == 'n' && p[1] == 'u' && p[2] == 'm' && p[3] == '=') {
            p += 4;
            size_t i = 0;
            while (*p && *p != '&' && *p != ' ' && i + 1 < num_len) {
                num[i++] = *p++;
            }
            num[i] = '\0';
            return i == 0 ? -1 : 0;
        }
        while (*p && *p != '&' && *p != ' ') {
            ++p;
        }
        if (*p == '&') {
            ++p;
        }
    }

    return -1;
}

static void send_response(const char *status, const char *body) {
    char header[160];
    char len_buf[32];
    size_t body_len = strlen(body);

    int_service_format((long long)body_len, len_buf, sizeof(len_buf));
    header[0] = '\0';
    memcpy(header, "HTTP/1.1 ", 9);
    size_t pos = 9;
    for (const char *p = status; *p && pos + 1 < sizeof(header); ++p) {
        header[pos++] = *p;
    }
    const char *mid = "\r\nContent-Type: text/plain\r\nContent-Length: ";
    for (const char *p = mid; *p && pos + 1 < sizeof(header); ++p) {
        header[pos++] = *p;
    }
    for (const char *p = len_buf; *p && pos + 1 < sizeof(header); ++p) {
        header[pos++] = *p;
    }
    const char *end = "\r\nConnection: close\r\n\r\n";
    for (const char *p = end; *p && pos + 1 < sizeof(header); ++p) {
        header[pos++] = *p;
    }
    header[pos] = '\0';

    g_send((const uint8_t *)header, strlen(header));
    g_send((const uint8_t *)body, body_len);
}

static void handle_request(void) {
    char num[64];
    char body[80];
    long long value;

    g_request[g_request_len] = '\0';
    if (find_num_param(g_request, num, sizeof(num)) != 0 || int_service_parse(num, &value) != 0) {
        send_response("400 Bad Request", "ERR\n");
        return;
    }

    int_service_format(value + 1, body, sizeof(body));
    size_t n = strlen(body);
    if (n + 1 < sizeof(body)) {
        body[n++] = '\n';
        body[n] = '\0';
    }

    console_puts("http: ");
    console_puts(num);
    console_puts(" -> ");
    console_puts(body);
    send_response("200 OK", body);
}

void http_app_init(tcp_app_send_fn send) {
    g_send = send;
}

uint16_t http_app_port(void) {
    return 80;
}

void http_app_on_connect(void) {
    g_request_len = 0;
}

void http_app_on_data(const uint8_t *data, size_t len) {
    for (size_t i = 0; i < len && g_request_len + 1 < sizeof(g_request); ++i) {
        g_request[g_request_len++] = (char)data[i];
        if (g_request_len >= 4 &&
            g_request[g_request_len - 4] == '\r' &&
            g_request[g_request_len - 3] == '\n' &&
            g_request[g_request_len - 2] == '\r' &&
            g_request[g_request_len - 1] == '\n') {
            handle_request();
            return;
        }
    }
}

void http_app_on_close(void) {
    g_request_len = 0;
}
