#pragma once
#include "stdint.h"

#define DHCP_DISCOVER 1
#define DHCP_OFFER 2

// source: https://dox.ipxe.org/structdhcphdr.html
struct DHCPHeader {
    uint8_t  op;           // DISCOVER = 1, OFFER = 2
    uint8_t  htype;        // ethernet = 1
    uint8_t  hlen;         // MAC length = 6
    uint8_t  hops;
    uint32_t xid;          // transaction id
    uint16_t secs;
    uint16_t flags;
    uint32_t ciaddr;       // clientchosen IP
    uint32_t yiaddr;       // your IP (server OFFER)
    uint32_t siaddr;
    uint32_t giaddr;
    uint8_t  chaddr[16];
    uint8_t  sname[64];
    uint8_t  file[128];
    uint32_t magic_cookie; // 0x63825363
};

// builds a a discover packet
void build_discover(DHCPHeader* dh, uint32_t requested_ip, uint8_t client_mac[6], uint32_t xid);

// builds an offer packet in response to gettign a discover
void build_offer(DHCPHeader* dh, uint32_t client_ip, uint8_t client_mac[6], uint32_t xid);

// parse an offer packet and get the offered IP address
int32_t parse_offer(DHCPHeader* dh, uint32_t* your_ip);