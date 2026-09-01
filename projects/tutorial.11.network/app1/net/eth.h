#pragma once

#include <stdint.h>

/* Ethernet header shape shared by experimental Ethernet helpers. */
typedef struct {
    uint8_t dst[6];
    uint8_t src[6];
    uint16_t ethertype;
} eth_hdr_t;
