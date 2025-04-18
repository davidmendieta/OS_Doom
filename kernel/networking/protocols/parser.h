

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
    uint8_t dest_mac[6];
    uint8_t src_mac[6];
    uint16_t ethertype; // e.g., 0x0800 = IPv4
};
#pragma pack(pop)


namespace Parser {

	void* send(const void* payload, size_t payload_len, size_t* out_packet_len);

	void* receive(const void* packet, size_t packet_len, size_t* out_payload_len);

	bool verify_receiver(const IPv4Header* ip_hdr);
	uint16_t checksum(const void* data, size_t len);
	void decrement_ttl(IPv4Header* ip_hdr);

//    void* ethernet_handle_packet(void * packet, uint32_t packet_length);


} // namespace Parser
