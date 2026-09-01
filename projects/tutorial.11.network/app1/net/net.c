#include "net.h"

#include "console.h"
#include "string.h"
#include "tcp.h"
#include "virtio_net.h"

#define ETH_TYPE_ARP 0x0806U
#define ETH_TYPE_IPV4 0x0800U
#define IP_PROTO_ICMP 1U
#define IP_PROTO_TCP 6U
#define MAX_FRAME 1600U

/* Ethernet header at the start of every frame delivered by VirtIO-net. */
typedef struct {
    uint8_t dst[6];
    uint8_t src[6];
    uint16_t type;
} __attribute__((packed)) eth_hdr_t;

/* Only Ethernet/IPv4 ARP packets are needed for QEMU user networking. */
typedef struct {
    uint16_t htype;
    uint16_t ptype;
    uint8_t hlen;
    uint8_t plen;
    uint16_t op;
    uint8_t sha[6];
    uint8_t spa[4];
    uint8_t tha[6];
    uint8_t tpa[4];
} __attribute__((packed)) arp_pkt_t;

/* Minimal IPv4 header without options. Pack it to match wire layout. */
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

static uint8_t g_mac[6];
static const uint8_t g_ip[4] = {10, 0, 2, 15};
static uint16_t g_ip_id = 1;

/* Network-byte-order helpers keep packed header access explicit. */
static uint16_t be16_read(const void *p) {
    const uint8_t *b = (const uint8_t *)p;
    return (uint16_t)((uint16_t)b[0] << 8 | b[1]);
}

static void be16_write(void *p, uint16_t value) {
    uint8_t *b = (uint8_t *)p;
    b[0] = (uint8_t)(value >> 8);
    b[1] = (uint8_t)value;
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

/* Validate IPv4, answer ping, and dispatch TCP payloads to tcp.c. */
static void ipv4_handle(const uint8_t *frame, size_t len, const uint8_t src_mac[6]) {
    if (len < sizeof(ipv4_hdr_t)) {
        return;
    }

    const ipv4_hdr_t *ip = (const ipv4_hdr_t *)frame;
    uint8_t version = (uint8_t)(ip->ver_ihl >> 4);
    uint8_t ihl = (uint8_t)((ip->ver_ihl & 0x0fU) * 4U);
    uint16_t total_len = be16_read(&ip->total_len);
    if (version != 4 || ihl < sizeof(ipv4_hdr_t) || total_len < ihl || total_len > len) {
        return;
    }

    if (memcmp(ip->dst, g_ip, sizeof(g_ip)) != 0) {
        return;
    }

    if (ipv4_checksum(frame, ihl) != 0) {
        return;
    }

    const uint8_t *payload = frame + ihl;
    size_t payload_len = total_len - ihl;

    if (ip->proto == IP_PROTO_ICMP) {
        /* ICMP echo reply is useful as a tiny network sanity check. */
        if (payload_len < 8 || payload[0] != 8) {
            return;
        }

        uint8_t reply[MAX_FRAME];
        eth_hdr_t *eth = (eth_hdr_t *)reply;
        ipv4_hdr_t *rip = (ipv4_hdr_t *)(reply + sizeof(*eth));
        uint8_t *rpayload = reply + sizeof(*eth) + sizeof(*rip);
        size_t rlen = sizeof(*eth) + sizeof(*rip) + payload_len;

        memcpy(eth->dst, src_mac, sizeof(eth->dst));
        memcpy(eth->src, g_mac, sizeof(eth->src));
        be16_write(&eth->type, ETH_TYPE_IPV4);

        rip->ver_ihl = 0x45;
        rip->tos = 0;
        be16_write(&rip->total_len, (uint16_t)(sizeof(*rip) + payload_len));
        be16_write(&rip->id, g_ip_id++);
        be16_write(&rip->frag_off, 0x4000U);
        rip->ttl = 64;
        rip->proto = IP_PROTO_ICMP;
        rip->checksum = 0;
        memcpy(rip->src, g_ip, sizeof(rip->src));
        memcpy(rip->dst, ip->src, sizeof(rip->dst));
        be16_write(&rip->checksum, ipv4_checksum(rip, sizeof(*rip)));

        memcpy(rpayload, payload, payload_len);
        rpayload[0] = 0;
        rpayload[2] = 0;
        rpayload[3] = 0;
        be16_write(&rpayload[2], ipv4_checksum(rpayload, payload_len));
        virtio_net_send(reply, rlen);
        return;
    }

    if (ip->proto == IP_PROTO_TCP) {
        /* TCP owns connection state; this layer only passes source addresses. */
        tcp_handle_segment(payload, payload_len, ip->src, src_mac);
    }
}

/* Reply to ARP requests for the static guest address 10.0.2.15. */
static void arp_handle(const uint8_t *frame, size_t len) {
    if (len < sizeof(arp_pkt_t)) {
        return;
    }

    const arp_pkt_t *arp = (const arp_pkt_t *)frame;
    if (be16_read(&arp->htype) != 1U || be16_read(&arp->ptype) != ETH_TYPE_IPV4 || arp->hlen != 6U || arp->plen != 4U) {
        return;
    }
    if (be16_read(&arp->op) != 1U || memcmp(arp->tpa, g_ip, sizeof(g_ip)) != 0) {
        return;
    }

    uint8_t reply[sizeof(eth_hdr_t) + sizeof(arp_pkt_t)];
    eth_hdr_t *eth = (eth_hdr_t *)reply;
    arp_pkt_t *rarp = (arp_pkt_t *)(reply + sizeof(*eth));

    memcpy(eth->dst, arp->sha, sizeof(eth->dst));
    memcpy(eth->src, g_mac, sizeof(eth->src));
    be16_write(&eth->type, ETH_TYPE_ARP);

    be16_write(&rarp->htype, 1U);
    be16_write(&rarp->ptype, ETH_TYPE_IPV4);
    rarp->hlen = 6U;
    rarp->plen = 4U;
    be16_write(&rarp->op, 2U);
    memcpy(rarp->sha, g_mac, sizeof(rarp->sha));
    memcpy(rarp->spa, g_ip, sizeof(rarp->spa));
    memcpy(rarp->tha, arp->sha, sizeof(rarp->tha));
    memcpy(rarp->tpa, arp->spa, sizeof(rarp->tpa));

    virtio_net_send(reply, sizeof(reply));
}

void net_init(const uint8_t mac[6]) {
    /* Net layer owns L2/L3 identity and initializes the TCP listener. */
    memcpy(g_mac, mac, sizeof(g_mac));
    tcp_init(g_mac, g_ip);

    console_puts("net: ip 10.0.2.15 port ");
    console_puthex(tcp_listen_port());
    console_puts("\n");
    console_puts("net: mac ");
    for (size_t i = 0; i < sizeof(g_mac); ++i) {
        uint8_t b = g_mac[i];
        const char hex[] = "0123456789abcdef";
        console_putc(hex[b >> 4]);
        console_putc(hex[b & 0x0f]);
        if (i + 1 != sizeof(g_mac)) {
            console_putc(':');
        }
    }
    console_puts("\n");
}

void net_handle_frame(const uint8_t *frame, size_t len) {
    /* Top-level Ethernet demux for packets delivered by VirtIO-net. */
    if (len < sizeof(eth_hdr_t)) {
        return;
    }

    const eth_hdr_t *eth = (const eth_hdr_t *)frame;
    uint16_t type = be16_read(&eth->type);

    if (type == ETH_TYPE_ARP) {
        arp_handle(frame + sizeof(*eth), len - sizeof(*eth));
        return;
    }

    if (type == ETH_TYPE_IPV4) {
        ipv4_handle(frame + sizeof(*eth), len - sizeof(*eth), eth->src);
    }
}
