// namespace Parser {
// 	void* Parse(char* buffer) {
// 		IPPacket ip;

// 		ip.VersionIHL = buffer[0];
// 	}
// }


#include "parser.h"
#include "../../heap.h"
#include "../../machine.h"
#include "../../debug.h"

const uint32_t my_ip = 0x0A000001;  // 10.0.0.1

static inline uint16_t ntohs(uint16_t netshort) {
    return (netshort << 8) | (netshort >> 8);
}

static inline uint16_t htons(uint16_t hostshort) {
    return (hostshort << 8) | (hostshort >> 8);
}

static inline uint32_t ntohl(uint32_t netlong) {
    return ((netlong & 0x000000FF) << 24) |
           ((netlong & 0x0000FF00) << 8)  |
           ((netlong & 0x00FF0000) >> 8)  |
           ((netlong & 0xFF000000) >> 24);
}

static inline uint32_t htonl(uint32_t hostlong) {
    return ((hostlong & 0x000000FF) << 24) |
           ((hostlong & 0x0000FF00) << 8)  |
           ((hostlong & 0x00FF0000) >> 8)  |
           ((hostlong & 0xFF000000) >> 24);
}



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


//
//
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



bool verify_receiver(const IPv4Header* ip_hdr) { 

    uint8_t version = ip_hdr->version_ihl >> 4;
    uint8_t ihl = ip_hdr->version_ihl & 0x0F; // header length in 32-bit words
    uint16_t ip_header_len = ihl * 4;

    if (version != 4 || ip_header_len < 20) {
        // Not IPv4 or malformed
        return false;
    }

    uint16_t total_len = ntohs(ip_hdr->total_length);
    uint32_t src_ip = ntohl(ip_hdr->src_ip);
    uint32_t dst_ip = ntohl(ip_hdr->dest_ip);
    uint8_t proto = ip_hdr->protocol;

    // udp = 17
    if (proto != 17) {
        return false;
    }

    // not my ip
    if (dst_ip != my_ip) {
        return false;
    }

//    Debug::printf("IPv4: %d.%d.%d.%d -> %d.%d.%d.%d, proto=%d\n",  
//        (src_ip >> 24) & 0xFF, (src_ip >> 16) & 0xFF,  
//        (src_ip >> 8) & 0xFF, src_ip & 0xFF,
//        (dst_ip >> 24) & 0xFF, (dst_ip >> 16) & 0xFF,
//        (dst_ip >> 8) & 0xFF, dst_ip & 0xFF,
//        proto
//    );

	return true; 
}

uint16_t checksum(const void* data, size_t len) {

    uint32_t acc = 0;
    const  uint16_t *ptr = (const uint16_t *)data;
    
    while (len > 1) {
        acc += *ptr++;
        len -= 2;
    }
    
    if (len > 0) {
        acc += *((const uint8_t *)ptr);
    }
    
    // Fold 32-bit sum to 16 bits
    while (acc >> 16)
        acc = (acc & 0xFFFF) + (acc >> 16);
    
    return ~((uint16_t)acc);
}

void decrement_ttl(IPv4Header* ip_hdr) {
    if (ip_hdr->ttl) { 
		ip_hdr->ttl--;
	}
}



//
//void* ethernet_handle_packet(void * packet, size_t packet_length) {
//    if (packet_length < sizeof(struct EthernetHeader) + sizeof(struct IPv4Header)) {
        // Too small to contain Ethernet + IPv4
//        return nullptr;
//    }

//    uint8_t* eth_frame = (uint8_t*)packet;
//    const struct EthernetHeader *eth = (const struct EthernetHeader *)eth_frame;

//    uint16_t ethertype = ntohs(eth->ethertype);
//    if (ethertype != 0x0800) {
        // Not IPv4
//        return nullptr;
//    }


//    const struct IPv4Header *ip = (const struct IPv4Header *)(eth_frame + sizeof(struct EthernetHeader));
//    size_t payload_len = 0;
//    void* receive( (const void*) ip, size_t packet_len, size_t* out_payload_len) {

//}









} // parser
