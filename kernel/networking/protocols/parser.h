#pragma once

#include <stdint.h>
#include <stddef.h>

#pragma pack(push, 1)
struct IPv4Header {
    uint8_t  version_ihl;
    uint8_t  type_of_service;
    uint16_t total_length;
    uint16_t identification;
    uint16_t flags_fragment;
    uint8_t  ttl;
    uint8_t  protocol;
    uint16_t header_checksum;
    uint32_t src_ip;
    uint32_t dest_ip;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct UDPHeader {
    uint16_t src_port;
    uint16_t dest_port;
    uint16_t length;
    uint16_t checksum; 
};
#pragma pack(pop)

namespace Parser {

	void* send(const void* payload, size_t payload_len, size_t* out_packet_len);

	void* receive(const void* packet, size_t packet_len, size_t* out_payload_len);

	bool verify_receiver(const IPv4Header* ip_hdr);
	uint16_t checksum(const void* data, size_t len);
	void decrement_ttl(IPv4Header* ip_hdr);

} // namespace Parser
