#include "tcp.h"

#include "console.h"
#include "string.h"
#include "tcp_app.h"
#include "virtio_net.h"

#define ETH_TYPE_IPV4 0x0800U
#define IP_PROTO_TCP 6U

#define TCP_FIN 0x01U
#define TCP_SYN 0x02U
#define TCP_RST 0x04U
#define TCP_PSH 0x08U
#define TCP_ACK 0x10U

#define LOCAL_TCP_ISN 0x10000000U
#define MAX_FRAME 1600U

/* Local copies of wire headers keep TCP independent from net.c internals. */
typedef struct {
    uint8_t dst[6];
    uint8_t src[6];
    uint16_t type;
} __attribute__((packed)) eth_hdr_t;

typedef struct {
    uint8_t ver_ihl;
    uint8_t tos;
    uint16_t total_len;
    uint16_t id;
    uint16_t frag_off;
    uint8_t ttl;
    uint8_t proto;
    uint16_t checksum;
    uint8_t src[4];
    uint8_t dst[4];
} __attribute__((packed)) ipv4_hdr_t;

typedef struct {
    uint16_t src_port;
    uint16_t dst_port;
    uint32_t seq;
    uint32_t ack;
    uint8_t data_off_reserved;
    uint8_t flags;
    uint16_t window;
    uint16_t checksum;
    uint16_t urgent;
} __attribute__((packed)) tcp_hdr_t;

/* This tiny stack supports one active connection at a time. */
typedef struct {
    uint8_t local_mac[6];
    uint8_t local_ip[4];
    uint8_t client_ip[4];
    uint8_t client_mac[6];
    uint16_t client_port;
    uint32_t client_next_seq;
    uint32_t server_next_seq;
    uint16_t ip_id;
    int connected;
    int established;
} tcp_conn_t;

static tcp_conn_t g_conn;

/* Packed wire headers must be read/written in network byte order. */
static uint16_t be16_read(const void *p) {
    const uint8_t *b = (const uint8_t *)p;
    return (uint16_t)((uint16_t)b[0] << 8 | b[1]);
}

static uint32_t be32_read(const void *p) {
    const uint8_t *b = (const uint8_t *)p;
    return ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) | ((uint32_t)b[2] << 8) | (uint32_t)b[3];
}

static void be16_write(void *p, uint16_t value) {
    uint8_t *b = (uint8_t *)p;
    b[0] = (uint8_t)(value >> 8);
    b[1] = (uint8_t)value;
}

static void be32_write(void *p, uint32_t value) {
    uint8_t *b = (uint8_t *)p;
    b[0] = (uint8_t)(value >> 24);
    b[1] = (uint8_t)(value >> 16);
    b[2] = (uint8_t)(value >> 8);
    b[3] = (uint8_t)value;
}

static uint32_t checksum_add(uint32_t sum, const void *data, size_t len) {
    const uint8_t *p = (const uint8_t *)data;
    while (len > 1) {
        sum += (uint32_t)((uint16_t)p[0] << 8 | p[1]);
        p += 2;
        len -= 2;
    }
    if (len) {
        sum += (uint32_t)p[0] << 8;
    }
    return sum;
}

static uint16_t checksum_finish(uint32_t sum) {
    while (sum >> 16) {
        sum = (sum & 0xffffU) + (sum >> 16);
    }
    return (uint16_t)~sum;
}

static uint16_t ipv4_checksum(const void *data, size_t len) {
    return checksum_finish(checksum_add(0, data, len));
}

static uint16_t tcp_checksum(const uint8_t src_ip[4], const uint8_t dst_ip[4], const tcp_hdr_t *tcp, const uint8_t *payload, size_t payload_len) {
    /* TCP checksum covers a pseudo-header plus TCP header and payload. */
    uint32_t sum = 0;
    uint16_t tcp_len = (uint16_t)(sizeof(*tcp) + payload_len);
    uint8_t pseudo[4] = {0, (uint8_t)IP_PROTO_TCP, (uint8_t)(tcp_len >> 8), (uint8_t)tcp_len};

    sum = checksum_add(sum, src_ip, 4);
    sum = checksum_add(sum, dst_ip, 4);
    sum = checksum_add(sum, pseudo, sizeof(pseudo));
    sum = checksum_add(sum, tcp, sizeof(*tcp));
    sum = checksum_add(sum, payload, payload_len);
    return checksum_finish(sum);
}

static void tcp_send_packet(uint16_t src_port, uint16_t dst_port, uint32_t seq, uint32_t ack, uint8_t flags, const uint8_t *payload, size_t payload_len) {
    /* Construct Ethernet + IPv4 + TCP in one contiguous frame for TX. */
    uint8_t frame[MAX_FRAME];
    eth_hdr_t *eth = (eth_hdr_t *)frame;
    ipv4_hdr_t *ip = (ipv4_hdr_t *)(frame + sizeof(*eth));
    tcp_hdr_t *tcp = (tcp_hdr_t *)(frame + sizeof(*eth) + sizeof(*ip));
    uint8_t *body = frame + sizeof(*eth) + sizeof(*ip) + sizeof(*tcp);
    size_t total_len = sizeof(*eth) + sizeof(*ip) + sizeof(*tcp) + payload_len;

    if (total_len > sizeof(frame)) {
        return;
    }

    memcpy(eth->dst, g_conn.client_mac, sizeof(eth->dst));
    memcpy(eth->src, g_conn.local_mac, sizeof(eth->src));
    be16_write(&eth->type, ETH_TYPE_IPV4);

    ip->ver_ihl = 0x45;
    ip->tos = 0;
    be16_write(&ip->total_len, (uint16_t)(sizeof(*ip) + sizeof(*tcp) + payload_len));
    be16_write(&ip->id, g_conn.ip_id++);
    be16_write(&ip->frag_off, 0x4000U);
    ip->ttl = 64;
    ip->proto = IP_PROTO_TCP;
    ip->checksum = 0;
    memcpy(ip->src, g_conn.local_ip, sizeof(ip->src));
    memcpy(ip->dst, g_conn.client_ip, sizeof(ip->dst));
    /* Checksums are computed in host registers and written big-endian. */
    be16_write(&ip->checksum, ipv4_checksum(ip, sizeof(*ip)));

    be16_write(&tcp->src_port, src_port);
    be16_write(&tcp->dst_port, dst_port);
    be32_write(&tcp->seq, seq);
    be32_write(&tcp->ack, ack);
    tcp->data_off_reserved = (uint8_t)(5U << 4);
    tcp->flags = flags;
    be16_write(&tcp->window, 0x4000U);
    tcp->checksum = 0;
    tcp->urgent = 0;

    if (payload_len) {
        memcpy(body, payload, payload_len);
    }

    be16_write(&tcp->checksum, tcp_checksum(ip->src, ip->dst, tcp, body, payload_len));
    virtio_net_send(frame, total_len);
}

static void tcp_send_data(const uint8_t *data, size_t len) {
    /* Application bytes are sent as one PSH+ACK segment. */
    tcp_send_packet(tcp_app_port(), g_conn.client_port, g_conn.server_next_seq, g_conn.client_next_seq, TCP_ACK | TCP_PSH, data, len);
    g_conn.server_next_seq += (uint32_t)len;
}

static void tcp_send_ack(void) {
    tcp_send_packet(tcp_app_port(), g_conn.client_port, g_conn.server_next_seq, g_conn.client_next_seq, TCP_ACK, NULL, 0);
}

static void tcp_send_synack(void) {
    tcp_send_packet(tcp_app_port(), g_conn.client_port, g_conn.server_next_seq, g_conn.client_next_seq, TCP_SYN | TCP_ACK, NULL, 0);
    g_conn.server_next_seq += 1U;
}

void tcp_init(const uint8_t mac[6], const uint8_t ip[4]) {
    /* Reset transport state and initialize the selected application handler. */
    memset(&g_conn, 0, sizeof(g_conn));
    memcpy(g_conn.local_mac, mac, sizeof(g_conn.local_mac));
    memcpy(g_conn.local_ip, ip, sizeof(g_conn.local_ip));
    g_conn.ip_id = 1;
    tcp_app_init(tcp_send_data);
}

uint16_t tcp_listen_port(void) {
    return tcp_app_port();
}

void tcp_handle_segment(const uint8_t *tcp_bytes, size_t len, const uint8_t src_ip[4], const uint8_t src_mac[6]) {
    /* This is a deliberately small TCP state machine for QEMU user networking. */
    if (len < sizeof(tcp_hdr_t)) {
        return;
    }

    const tcp_hdr_t *tcp = (const tcp_hdr_t *)tcp_bytes;
    uint16_t src_port = be16_read(&tcp->src_port);
    uint16_t dst_port = be16_read(&tcp->dst_port);
    uint32_t seq = be32_read(&tcp->seq);
    uint32_t ack = be32_read(&tcp->ack);
    uint8_t data_offset = (uint8_t)((tcp->data_off_reserved >> 4) * 4U);
    uint8_t flags = tcp->flags;
    size_t header_len = data_offset;

    if (dst_port != tcp_app_port()) {
        return;
    }

    if ((flags & TCP_RST) != 0) {
        g_conn.connected = 0;
        g_conn.established = 0;
        tcp_app_on_close();
        return;
    }

    if ((flags & TCP_SYN) != 0 && (flags & TCP_ACK) == 0) {
        /* Passive open: remember peer identity and answer with SYN-ACK. */
        memcpy(g_conn.client_ip, src_ip, sizeof(g_conn.client_ip));
        memcpy(g_conn.client_mac, src_mac, sizeof(g_conn.client_mac));
        g_conn.client_port = src_port;
        g_conn.client_next_seq = seq + 1U;
        g_conn.server_next_seq = LOCAL_TCP_ISN;
        g_conn.connected = 1;
        g_conn.established = 0;
        tcp_app_on_connect();
        tcp_send_synack();
        return;
    }

    if (!g_conn.connected || src_port != g_conn.client_port) {
        return;
    }

    if ((flags & TCP_ACK) != 0 && ack == g_conn.server_next_seq) {
        g_conn.established = 1;
    }

    if (header_len < sizeof(tcp_hdr_t) || header_len > len) {
        tcp_send_ack();
        return;
    }

    const uint8_t *payload = tcp_bytes + header_len;
    size_t payload_len = len - header_len;
    uint32_t expected = g_conn.client_next_seq;

    if (payload_len == 0) {
        /* Pure ACKs advance no application data. FIN closes the connection. */
        if ((flags & TCP_FIN) != 0 && seq == expected) {
            g_conn.client_next_seq += 1U;
            tcp_send_ack();
            g_conn.connected = 0;
            g_conn.established = 0;
            tcp_app_on_close();
        }
        return;
    }

    if (seq < expected) {
        /* Trim retransmitted bytes instead of dropping the useful tail. */
        uint32_t already_seen = expected - seq;
        if (already_seen >= payload_len) {
            tcp_send_ack();
            return;
        }
        payload += already_seen;
        payload_len -= already_seen;
        seq = expected;
    }

    if (seq > expected) {
        tcp_send_ack();
        return;
    }

    g_conn.client_next_seq += (uint32_t)payload_len;
    /* Once transport ordering is satisfied, hand raw bytes to HTTP/telnet. */
    tcp_app_on_data(payload, payload_len);

    if ((flags & TCP_FIN) != 0) {
        g_conn.client_next_seq += 1U;
        tcp_send_ack();
        g_conn.connected = 0;
        g_conn.established = 0;
        tcp_app_on_close();
        return;
    }

    tcp_send_ack();
}
