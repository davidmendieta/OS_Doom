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
struct UDPPacket {
	uint16_t src_port[2];         // Source Port
	uint16_t dest_port[2];		  // Destination Port

	uint16_t total_length[2];     // Length
	uint16_t header_checksum[2];  // Checksum
								  
	void* payload;                // Payload
}; // 8 bytes + payload
#pragma pack(pop)

struct FullPacket {
	IPPacket;
	UDPPacket;
}

namespace Parser {

	extern void* Parse(void* buffer);

}

