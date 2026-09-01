#include "eth.h"

/* Helper retained for future Ethernet code that wants explicit endian swaps. */
uint16_t ethertype_host_to_net(uint16_t value) {
    return (uint16_t)((value << 8) | (value >> 8));
}
