
#pragma once

#include <stdint.h>
#include <stddef.h>

#pragma pack(push, 1)
struct IPv4Header {
    uint8_t version_ihl;       // Version (4 bits) + IHL (4 bits)
    uint8_t type_of_service;   // Type of service (usually 0)
    uint16_t total_length;     // Total length (header + data 2 bytes)
    uint16_t identification;   // Identification
    uint16_t flags_fragment;   // Flags (3 bits) + Fragment offset (13 bits)
    uint8_t ttl;               // Time to live
    uint8_t protocol;          // Protocol (17 = UDP)
    uint16_t header_checksum;  // Header checksum
    uint32_t src_ip;           // Source IP address
    uint32_t dest_ip;          // Destination IP address
}; // 20 bytes
#pragma pack(pop)


#pragma pack(push, 1)
struct UDPHeader {
    uint16_t src_port;
    uint16_t dest_port;
    uint16_t length;
    uint16_t checksum; 
};
#pragma pack(pop)

#pragma pack(push, 1)
struct EthernetHeader {
    char dest_mac[6];
    char src_mac[6];
    uint16_t ethertype; // e.g., 0x0800 = IPv4
};
#pragma pack(pop)

#pragma pack(push, 1)
struct PseudoHeader {
    uint32_t src_ip;
    uint32_t dst_ip;
    uint8_t  zero;
    uint8_t  protocol;
    uint16_t udp_length;
};
#pragma pack(pop)

static inline uint16_t ntohs(uint16_t netshort) {
    return (netshort << 8) | (netshort >> 8);
}

static inline uint16_t htons(uint16_t hostshort) {
    return (hostshort << 8) | (hostshort >> 8);
}

static inline uint32_t ntohl(uint32_t netlong) {
    return ((netlong & 0x000000FF) << 24) | ((netlong & 0x0000FF00) << 8) |
           ((netlong & 0x00FF0000) >> 8) | ((netlong & 0xFF000000) >> 24);
}

static inline uint32_t htonl(uint32_t hostlong) {
    return ((hostlong & 0x000000FF) << 24) | ((hostlong & 0x0000FF00) << 8) |
           ((hostlong & 0x00FF0000) >> 8) | ((hostlong & 0xFF000000) >> 24);
}

namespace Parser {

	char* encode_ip_udp(const void* payload, size_t payload_len, uint32_t src_ip, char* src_mac, uint16_t src_port, uint32_t dest_ip, char* dest_mac, uint16_t dest_port, uint32_t* out_packet_len);

	char* decode_ip_udp(const void* packet, size_t packet_len, size_t* out_payload_len,uint32_t* src_ip, uint16_t* src_port, uint16_t* dest_port);

	bool verify_receiver(const IPv4Header* ip_hdr);
	uint16_t checksum(const void* data, size_t len);
	void decrement_ttl(IPv4Header* ip_hdr);

    uint16_t fold_checksum(uint32_t sum);
    uint16_t udp_checksum(const IPv4Header* ip, const UDPHeader* udp, const char* payload, size_t payload_len);
    
    char* encode_ip_tcp(const void* payload, size_t payload_len, uint32_t src_ip,
        char* src_mac, uint16_t src_port, uint32_t dest_ip,
        char* dest_mac, uint16_t dest_port,
        uint32_t* out_packet_len);
    void receive_tcp(const IPv4Header* ip, const TCPHeader* tcp, size_t payload_len);
} // namespace Parser
