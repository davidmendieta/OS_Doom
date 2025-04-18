#include "parser.h"
#include "../heap.h"
#include "../machine.h"
#include "./drivers/rtl8139.h" // to access ip address
#include "networking.h" // to get the driver device

namespace Parser {

void* send(const void* payload, size_t payload_len, size_t* out_packet_len) {
    size_t udp_len = sizeof(UDPHeader) + payload_len;
    size_t ip_len  = sizeof(IPv4Header) + udp_len;

    uint8_t* buf = (uint8_t*) malloc(ip_len);

    IPv4Header* ip = (IPv4Header*)buf;
    ip->version_ihl     = (4 << 4) | (sizeof(IPv4Header)/4);
    ip->type_of_service = 0;
    ip->total_length    = (uint16_t)ip_len;
    ip->identification  = 0;
    ip->flags_fragment  = 0x4000;
    ip->ttl             = 64;
    ip->protocol        = 17;
    ip->header_checksum = 0;
    ip->src_ip          = 0;
    ip->dest_ip         = 0;

    UDPHeader* udp = (UDPHeader*)(buf + sizeof(IPv4Header));
    udp->src_port   = 0;
    udp->dest_port  = 0;
    udp->length     = (uint16_t)udp_len;
    udp->checksum   = 0;

    memcpy(buf + sizeof(IPv4Header) + sizeof(UDPHeader), payload, payload_len);

    *out_packet_len = ip_len;
    return buf;
}

void* receive(const void* packet, size_t packet_len, size_t* out_payload_len) {
    if (packet_len < sizeof(IPv4Header) + sizeof(UDPHeader)) {
        return 0;
	}

    const IPv4Header* ip = (const IPv4Header*)packet;
    if (!verify_receiver(ip)) {
        return 0;
	}

    size_t ihl_bytes = (ip->version_ihl & 0x0F) * 4;
    if (packet_len < ihl_bytes + sizeof(UDPHeader)) {
        return 0;
	}

    const UDPHeader* udp = (const UDPHeader*) ((const uint8_t*)packet + ihl_bytes);

    size_t udp_len = udp->length;
    if (packet_len < ihl_bytes + udp_len) {
        return 0;
	}

    size_t payload_len = udp_len - sizeof(UDPHeader);
    void* payload = malloc(payload_len);

    memcpy(payload, (const uint8_t*)packet + ihl_bytes + sizeof(UDPHeader), payload_len);

    *out_payload_len = payload_len;
    return payload;
}

bool verify_receiver(const IPv4Header* /*ip_hdr*/ ip_hdr) {
    return ip_hdr->dest_ip == rtl8139->my_ip;
	// return true; 
}

uint16_t checksum(const void* /*data*/, size_t /*len*/) {
	return 0; 
}

void decrement_ttl(IPv4Header* ip_hdr) {
    if (ip_hdr->ttl) { 
		ip_hdr->ttl--;
	}
}

} // parser
