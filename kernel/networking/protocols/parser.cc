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

// const uint32_t src_ip = 0x0A000001;  // 10.0.0.1
// const uint32_t dest_ip = 0x0A000002;  // 10.0.0.2
// const uint16_t src_port = 1234;  // source udp port
// const uint16_t dest_port = 5678;  // dest udp port

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

// simple 16‑bit ones‑complement adder/fold
uint16_t fold_checksum(uint32_t sum) {
    while (sum >> 16) sum = (sum & 0xffff) + (sum >> 16);
    return (uint16_t)~sum;
}


//void* send(const void* payload, size_t payload_len, size_t* out_packet_len) {
//    size_t udp_len = sizeof(UDPHeader) + payload_len;
//    size_t ip_len  = sizeof(IPv4Header) + udp_len;

//    auto* buf = (uint8_t*) malloc(ip_len);
//    auto* ip = (IPv4Header*)buf;

//    ip->version_ihl     = (4 << 4) | (sizeof(IPv4Header)/4);
//    ip->type_of_service = 0;
//    ip->total_length    = (uint16_t)ip_len;
//    ip->identification  = 0;
//    ip->flags_fragment  = 0x4000;
//    ip->ttl             = 64;
//    ip->protocol        = 17;
//    ip->header_checksum = 0;
//    ip->src_ip          = 0;
//    ip->dest_ip         = 0;

//    UDPHeader* udp = (UDPHeader*)(buf + sizeof(IPv4Header));
//    udp->src_port   = 0;
//    udp->dest_port  = 0;
//    udp->length     = (uint16_t)udp_len;
//    udp->checksum   = 0;

//    memcpy(buf + sizeof(IPv4Header) + sizeof(UDPHeader), payload, payload_len);

//    *out_packet_len = ip_len;
//    return buf;
//}


//
// send output (void* buf) is IPV4 + UDP (no ethernet header)
//
void* send(const void* payload, size_t payload_len, uint32_t src_ip, uint16_t src_port,uint32_t dest_ip, uint16_t dest_port, size_t* out_packet_len) {
    size_t udp_len = sizeof(UDPHeader) + payload_len;
    size_t ip_len  = sizeof(IPv4Header) + udp_len;

    auto* buf = (uint8_t*)malloc(ip_len);
    auto* ip  = (IPv4Header*)buf;

    // --- fill IPv4 header (all multi‑byte fields in network order) ---
    ip->version_ihl     = (4 << 4) | (sizeof(IPv4Header)/4);
    ip->type_of_service = 0;
    ip->total_length    = htons((uint16_t)ip_len);
    ip->identification  = htons(0);
    ip->flags_fragment  = htons(0x4000);
    ip->ttl             = 64;
    ip->protocol        = 17;                   // UDP
    ip->header_checksum = 0;                    // MUST zero before computing
    ip->src_ip          = htonl(src_ip);
    ip->dest_ip         = htonl(dest_ip);

    // compute IPv4 header checksum over just the header
    ip->header_checksum = checksum(ip, sizeof(IPv4Header));

    // --- fill UDP header ---
    auto* udp = (UDPHeader*)(buf + sizeof(IPv4Header));
    udp->src_port  = htons(src_port);
    udp->dest_port = htons(dest_port);
    udp->length    = htons((uint16_t)udp_len);
    udp->checksum  = 0;                          // zero before computing

    // copy payload
    memcpy(buf + sizeof(IPv4Header) + sizeof(UDPHeader),
           payload, payload_len);

    // compute UDP checksum (including pseudo‑header)
    udp->checksum = udp_checksum(ip, udp,
                                 buf + sizeof(IPv4Header) + sizeof(UDPHeader),
                                 payload_len);

    *out_packet_len = ip_len;
    return buf;
}



//
// packet is ethernet packet: ethernet header + IPv4 + UDP 
//
void* receive(const void* packet, size_t packet_len, size_t* out_payload_len,uint32_t* src_ip, uint16_t* src_port, uint16_t* dest_port) {

    if (packet_len < sizeof(struct EthernetHeader) + sizeof(struct IPv4Header) + sizeof(UDPHeader)) {
        // Too small to contain Ethernet + IPv4
        return 0;
    }

    uint8_t* eth_frame = (uint8_t*)packet;
    const struct EthernetHeader *eth = (const struct EthernetHeader *)eth_frame;

    uint16_t ethertype = ntohs(eth->ethertype);
    if (ethertype != 0x0800) {
        // Not IPv4
        return 0;
    }

    const IPv4Header *ip = (const IPv4Header *)(eth_frame + sizeof(EthernetHeader));
    size_t ihl_bytes = (ip->version_ihl & 0x0F) * 4;


    // 1) verify IPv4 header checksum
    if (checksum(ip, ihl_bytes) != 0) {
        return 0;
    }

    // 2) protocol & IP‑dest checks
    if (!verify_receiver(ip)) {
        return 0;
	}

//    if (packet_len < sizeof(IPv4Header) + sizeof(UDPHeader)) {
//        return 0;
//	}

//    const IPv4Header* ip = (const IPv4Header*)packet;
//    if (!verify_receiver(ip)) {
//        return 0;
//	}

//    if (packet_len < ihl_bytes + sizeof(UDPHeader)) {
//        return 0;
//	}

    // 3) locate UDP header and bounds‑check
    const UDPHeader* udp = (const UDPHeader*) ((const uint8_t*)ip + ihl_bytes);
    size_t udp_len = ntohs(udp->length);
    if (packet_len < ihl_bytes + udp_len) {
        return 0;
	}

    size_t payload_len = udp_len - sizeof(UDPHeader);
    const uint8_t* payload_ptr = (const uint8_t*)udp + sizeof(UDPHeader);

    // 4) verify UDP checksum
    if (udp_checksum(ip, udp, payload_ptr, payload_len) != 0) {
        return 0;
    }

    // 5) finally, extract payload
    void* payload = malloc(payload_len);

    memcpy(payload, payload_ptr, payload_len);

    //SOCKET STUFF
    *src_port = ntohs(udp->src_port);
    *dest_port = ntohs(udp->dest_port);
    *src_ip = ntohs(ip->src_ip);

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
//    uint16_t chksum = ip_hdr->header_checksum;

    // validate checksum
//    uint16_t checksum(const void* data, size_t len);

    // udp = 17
    if (proto != 17) {
        return false;
    }

    // not my ip
    if (dst_ip != src_ip) {
        return false;
    }

    Debug::printf("IPv4: %d.%d.%d.%d -> %d.%d.%d.%d, len=%d, proto=%d\n",  
        (src_ip >> 24) & 0xFF, (src_ip >> 16) & 0xFF,  
        (src_ip >> 8) & 0xFF, src_ip & 0xFF,
        (dst_ip >> 24) & 0xFF, (dst_ip >> 16) & 0xFF,
        (dst_ip >> 8) & 0xFF, dst_ip & 0xFF,
        total_len, proto
    );

	return true; 
}

uint16_t checksum(const void* data, size_t len) {
    uint32_t acc = 0;
    const uint16_t* ptr = (const uint16_t*)data;
    while (len > 1) {
        acc += *ptr++;
        len -= 2;
    }
    if (len) {
        acc += *(const uint8_t*)ptr;
    }
    return fold_checksum(acc);
}

void decrement_ttl(IPv4Header* ip_hdr) {
    if (ip_hdr->ttl) { 
		ip_hdr->ttl--;
	}
}

// build and checksum the UDP packet (pseudo‑header + UDP header + payload)
uint16_t udp_checksum(const IPv4Header* ip, const UDPHeader* udp,
    const uint8_t* payload, size_t payload_len) {

    PseudoHeader ph = {};
    ph.src_ip     = ip->src_ip;                  // already in network byte order
    ph.dst_ip     = ip->dest_ip;                 // ditto
    ph.zero       = 0;
    ph.protocol   = ip->protocol;                // usually 17
    ph.udp_length = udp->length;                 // network byte order

    // sum pseudo‑header
    uint32_t sum = 0;
    sum += checksum(&ph, sizeof(ph));            // fold inside

    // sum UDP header
    sum += checksum(udp, sizeof(UDPHeader));

    // sum payload (and pad if odd)
    sum += checksum(payload, payload_len);

    return fold_checksum(sum);
}


} // parser