#include "libc.h"   // memcpy, bzero
#include "dhcp.h"

// client -> server
void build_discover(DHCPHeader* dh, uint32_t requested_ip, uint8_t client_mac[6], uint32_t xid) {
    // initially zero out the header
    memset(dh, 0, sizeof(*dh));
    dh->op = DHCP_DISCOVER; // mark as discover
    dh->htype = 1; // ethernet
    dh->hlen = 6; // mac length
    dh->xid = xid; // transaction ID
    dh->ciaddr = requested_ip; // request this IP
    dh->magic_cookie = 0x63825363; // DHCP magic cookie
    memcpy(dh->chaddr, client_mac, 6); // then copy the client MAC onto the field
}

// server -> client
void build_offer(DHCPHeader* dh, uint32_t client_ip, uint8_t client_mac[6], uint32_t xid) {
    memset(dh, 0, sizeof(*dh));
    dh->op = DHCP_OFFER;
    dh->htype = 1;
    dh->hlen = 6;
    dh->xid = xid;
    dh->yiaddr = client_ip;
    dh->magic_cookie = 0x63825363;
    memcpy(dh->chaddr, client_mac, 6);
}

// client side, client parses the incoming offer from dhcp
int32_t parse_offer(DHCPHeader* dh, uint32_t* your_ip) {
    if (dh->op == DHCP_OFFER) {
        *your_ip = dh->yiaddr;
        return 0;
    }
    return -1;
}