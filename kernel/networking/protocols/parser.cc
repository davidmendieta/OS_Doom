// namespace Parser {
// 	void* Parse(char* buffer) {
// 		IPPacket ip;

// 		ip.VersionIHL = buffer[0];
// 	}
// }

#include "parser.h"
#include "tcp.h"
#include "../../debug.h"
#include "../../heap.h"
#include "../../machine.h"
#include "../interface/network_interface.h"
#include "../networking.h"

// const uint32_t src_ip = 0x0A000001;  // 10.0.0.1
// const uint32_t dest_ip = 0x0A000002;  // 10.0.0.2
// const uint16_t src_port = 1234;  // source udp port
// const uint16_t dest_port = 5678;  // dest udp port



namespace Parser {

// simple 16‑bit ones‑complement adder/fold
uint16_t fold_checksum(uint32_t sum) {
    while (sum >> 16) sum = (sum & 0xffff) + (sum >> 16);
    return (uint16_t)~sum;
}

// build and checksum the UDP packet (pseudo‑header + UDP header + payload)
uint16_t udp_checksum(const IPv4Header* ip, const UDPHeader* udp,
                      const char* payload, size_t payload_len) {
    PseudoHeader ph = {};
    ph.src_ip = ip->src_ip;   // already in network byte order
    ph.dst_ip = ip->dest_ip;  // ditto
    ph.zero = 0;
    ph.protocol = ip->protocol;   // usually 17
    ph.udp_length = udp->length;  // network byte order

    // sum pseudo‑header
    uint32_t sum = 0;
    // sum += checksum(&ph, sizeof(ph));  // fold inside
    sum += (uint32_t)(~checksum(&ph,       sizeof(ph)));

    // sum UDP header
    // sum += checksum(udp, sizeof(UDPHeader));
    sum += (uint32_t)(~checksum(udp,       sizeof(UDPHeader)));

    // sum payload (and pad if odd)
    // sum += checksum(payload, payload_len);
    sum += (uint32_t)(~checksum(payload,   payload_len));


    return fold_checksum(sum);
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

// FUNCTION Wrap payload with UDP
// FUNCTION Wrap something with IP
// FUNCTION Wrap something with Ethernet

char* encode_ip_udp(const void* payload, size_t payload_len, uint32_t src_ip,
                    char* src_mac, uint16_t src_port, uint32_t dest_ip,
                    char* dest_mac, uint16_t dest_port,
                    uint32_t* out_packet_len) {

    size_t udp_len = sizeof(UDPHeader) + payload_len;
    size_t ip_len = sizeof(IPv4Header) + udp_len;
    size_t ethernet_len = sizeof(EthernetHeader) + ip_len;

    auto* buf = (char*)malloc(ethernet_len);

    // --- fill Ethernet Header ---
    auto* ethernet = (EthernetHeader*)buf;
    memcpy(ethernet->src_mac, src_mac, 6);
    memcpy(ethernet->dest_mac, dest_mac, 6);
    ethernet->ethertype = IPV4_VALUE;

    auto* ip = (IPv4Header*)(buf + sizeof(EthernetHeader));

    // --- fill IPv4 header (all multi‑byte fields in network order) ---
    ip->version_ihl = (4 << 4) | (sizeof(IPv4Header) / 4);
    ip->type_of_service = 0;
    ip->total_length = htons((uint16_t)ip_len);
    ip->identification = htons(0);
    ip->flags_fragment = htons(0x4000);
    ip->ttl = 64;
    ip->protocol = 17;        // UDP
    ip->header_checksum = 0;  // MUST zero before computing
    ip->src_ip = htonl(src_ip);
    ip->dest_ip = htonl(dest_ip);

    // compute IPv4 header checksum over just the header
    ip->header_checksum = checksum(ip, sizeof(IPv4Header));

    // --- fill UDP header ---
    auto* udp = (UDPHeader*)(buf + sizeof(EthernetHeader) + sizeof(IPv4Header));
    udp->src_port = htons(src_port);
    udp->dest_port = htons(dest_port);
    udp->length = htons((uint16_t)udp_len);
    udp->checksum = 0;  // zero before computing

    // copy payload
    memcpy(buf + sizeof(EthernetHeader) + sizeof(IPv4Header) + sizeof(UDPHeader), payload, payload_len);

    // compute UDP checksum (including pseudo‑header)
    udp->checksum = udp_checksum(
        ip, udp, buf + sizeof(IPv4Header) + sizeof(UDPHeader), payload_len);

    *out_packet_len = ethernet_len;
    return buf;
}

//
// packet is ethernet packet: ethernet header + IPv4 + UDP
//
char* decode_ip_udp(const void* packet, size_t packet_len,
                    size_t* out_payload_len, uint32_t* src_ip,
                    uint16_t* src_port, uint16_t* dest_port) {
    if (packet_len < sizeof(struct IPv4Header) + sizeof(UDPHeader)) {
        // Too small to contain Ethernet + IPv4
        // return 0;
        Debug::printf("PACKET LENGTH PROBLEM\n");
        return nullptr;
    }

    const IPv4Header* ip = (const IPv4Header*)(packet);
    size_t ihl_bytes = (ip->version_ihl & 0x0F) * 4;

    // 1) verify IPv4 header checksum
    if (checksum(ip, ihl_bytes) != 0) {
        // return 0;
        Debug::printf("CHECK SUM FAILED\n");
        return nullptr;
    }

    // Verify ttl
    if (ip->ttl <= 0){
        Debug::printf("IP TTL FAILED\n");
        return nullptr;
    }
    decrement_ttl((IPv4Header*) ip);

    // 2) protocol & IP‑dest checks
    if (!verify_receiver(ip)) {
        // return 0;
        Debug::printf("VERIFY RECEIVER FAILED\n");
        return nullptr;
    }

    // 3) locate UDP header and bounds‑check
    const UDPHeader* udp = (const UDPHeader*)((const uint8_t*)ip + ihl_bytes);
    size_t udp_len = ntohs(udp->length);
    if (packet_len < ihl_bytes + udp_len) {
        // return 0;
        Debug::printf("PACKET LEN FAILED\n");
        return nullptr;
    }

    size_t payload_len = udp_len - sizeof(UDPHeader);
    const char* payload_ptr = (const char*)udp + sizeof(UDPHeader);

    // // 4) verify UDP checksum //! THIS ONE DOESN'T WORK
    // if (udp_checksum(ip, udp, payload_ptr, payload_len) != 0) {
    //     // return 0;
    //     Debug::printf("UDP CHECKSUM FAILED\n");
    //     return nullptr;
    // }

    // extract metadata
    *src_ip = ntohl(ip->src_ip);
    
    *src_port = ntohs(udp->src_port);
    *dest_port = ntohs(udp->dest_port);
   
    
   // *out_payload_len = payload_len; //TODO: Not Needed...-David

    // 5) finally, extract payload
    char* payload = (char*)malloc(payload_len);

    memcpy(payload, payload_ptr, payload_len);

    // // SOCKET STUFF
    // *src_port = ntohs(udp->src_port);
    // *dest_port = ntohs(udp->dest_port);
    // *src_ip = ntohs(ip->src_ip);

    *out_payload_len = payload_len;

    Debug::printf("RETURNING PAYLOAD\n");
    return payload;
}

bool verify_receiver(const IPv4Header* ip_hdr) {
    uint8_t version = ip_hdr->version_ihl >> 4;
    uint8_t ihl = ip_hdr->version_ihl & 0x0F;  // header length in 32-bit words
    uint16_t ip_header_len = ihl * 4;

    if (version != 4 || ip_header_len < 20) {
        // Not IPv4 or malformed
        return false;
    }

    //uint16_t total_len = ntohs(ip_hdr->total_length);
    //uint32_t src_ip = ntohl(ip_hdr->src_ip);
    uint32_t dst_ip = ntohl(ip_hdr->dest_ip);
    uint8_t proto = ip_hdr->protocol;

    // udp = 17
    if (proto != 17) {
        return false;
    }

    // not my ip
    if (dst_ip != network_interface->get_ip()) {
        return false;
    }

	/*
    Debug::printf("IPv4: %d.%d.%d.%d -> %d.%d.%d.%d, len=%d, proto=%d\n",
                  (src_ip >> 24) & 0xFF, (src_ip >> 16) & 0xFF,
                  (src_ip >> 8) & 0xFF, src_ip & 0xFF, (dst_ip >> 24) & 0xFF,
                  (dst_ip >> 16) & 0xFF, (dst_ip >> 8) & 0xFF, dst_ip & 0xFF,
                  total_len, proto);
				  */

    return true;
}

void decrement_ttl(IPv4Header* ip_hdr) {
    if (ip_hdr->ttl) {
        ip_hdr->ttl--;
    }
}


/*   --- TCP ----   */ 
uint16_t tcp_checksum(const IPv4Header* ip, const TCPHeader* tcp,
    const char* payload, size_t payload_len) {
        
    PseudoHeader ph = {};
    ph.src_ip     = ip->src_ip;
    ph.dst_ip     = ip->dest_ip;
    ph.zero       = 0;
    ph.protocol   = 6; // TCP
    ph.udp_length = htons(sizeof(TCPHeader) + payload_len);

    uint32_t sum = 0;
    sum += (~checksum(&ph, sizeof(ph))) & 0xFFFF;
    sum += (~checksum(tcp, sizeof(TCPHeader))) & 0xFFFF;
    sum += (~checksum(payload, payload_len)) & 0xFFFF;

    return fold_checksum(sum);
}

char* encode_ip_tcp(const void* payload, size_t payload_len, uint32_t src_ip,
    char* src_mac, uint16_t src_port, uint32_t dest_ip,
    char* dest_mac, uint16_t dest_port,
    uint32_t* out_packet_len) {

    size_t tcp_len = sizeof(TCPHeader) + payload_len;
    size_t ip_len = sizeof(IPv4Header) + tcp_len;
    size_t ethernet_len = sizeof(EthernetHeader) + ip_len;

    auto* buf = (char*)malloc(ethernet_len);

    // fill Ethernet Header
    auto* ethernet = (EthernetHeader*)buf;
    memcpy(ethernet->src_mac, src_mac, 6);
    memcpy(ethernet->dest_mac, dest_mac, 6);
    ethernet->ethertype = htons(IPV4_VALUE);

    auto* ip = (IPv4Header*)(buf + sizeof(EthernetHeader));

    // fill IPv4 header
    ip->version_ihl = (4 << 4) | (sizeof(IPv4Header) / 4);
    ip->type_of_service = 0;
    ip->total_length = htons((uint16_t)ip_len);
    ip->identification = htons(0);
    ip->flags_fragment = htons(0x4000);
    ip->ttl = 64;
    ip->protocol = 6; // 6 for TCP
    ip->header_checksum = 0;
    ip->src_ip = htonl(src_ip);
    ip->dest_ip = htonl(dest_ip);

    ip->header_checksum = checksum(ip, sizeof(IPv4Header));

    // fill TCP header
    auto* tcp = (TCPHeader*)(buf + sizeof(EthernetHeader) + sizeof(IPv4Header));
    memcpy(tcp, payload, sizeof(TCPHeader));

    // copy TCP payload if any
    if (payload_len > 0) {
        memcpy(buf + sizeof(EthernetHeader) + sizeof(IPv4Header) + sizeof(TCPHeader),
            (const char*)payload + sizeof(TCPHeader),
            payload_len);
    }

    // TCP checksum
    tcp->checksum = 0;
    tcp->checksum = tcp_checksum(ip, tcp, buf + sizeof(EthernetHeader) + sizeof(IPv4Header)
                                + sizeof(TCPHeader), payload_len);

    *out_packet_len = ethernet_len;
    return buf;
}

TCB_tcp* find_tcb(uint16_t src_port, uint16_t dest_port, uint32_t src_ip) {
    // Ok this is as far as this implementation will go.
    // We normally would have a hashmap of active connections
    // here, in which we would verify if we have to establish
    // a new connection or just fetch it.

    // For now, assume we only allow 1 connection, kinda like a global TCB.
    static TCB_tcp tcb;

    if (tcb.state == TCPState::CLOSED) {
        tcb.local_port = dest_port;
        tcb.remote_port = src_port;
        tcb.remote_ip = src_ip;
        tcb.state = TCPState::LISTEN; // or SYN_SENT depending on role
    }

    return &tcb;
}

void receive_tcp(const IPv4Header* ip, const TCPHeader* tcp, size_t payload_len) {
    uint16_t src_port = ntohs(tcp->src_port);
    uint16_t dest_port = ntohs(tcp->dest_port);
    uint32_t seq_num = ntohl(tcp->seq_num);
    // uint32_t ack_num = ntohl(tcp->ack_num);

    TCB_tcp* conn = find_tcb(src_port, dest_port, ntohl(ip->src_ip));
    if (!conn) {
        Debug::printf("TCP: No TCB found or created\n");
        return;
    }

    uint8_t flags = tcp->flags;

    switch (conn->state) {
    case TCPState::LISTEN:
        if (flags & 0x02) { // SYN received
            Debug::printf("TCP: Received SYN in LISTEN, sending SYN+ACK\n");

            conn->state = TCPState::SYN_RECEIVED;
            conn->ack_num = seq_num + 1;
            conn->seq_num = 100; // bc why not

            send_tcp_syn_ack(conn);
        }
        break;

    case TCPState::SYN_SENT:
        if ((flags & 0x12) == 0x12) { // SYN+ACK received
            Debug::printf("TCP: Received SYN+ACK in SYN_SENT, sending ACK\n");

            conn->state = TCPState::ESTABLISHED;
            conn->ack_num = seq_num + 1;
            conn->seq_num += 1; // We sent SYN already

            send_tcp_ack(conn);
        }
        break;

    case TCPState::SYN_RECEIVED:
    // ACK received
    if (flags & 0x10) { 
            Debug::printf("TCP: Received ACK in SYN_RECEIVED, connection ESTABLISHED\n");

            conn->state = TCPState::ESTABLISHED;
        }
        break;

    default:
        Debug::printf("TCP: Packet received in unexpected state\n");
        break;
    }
}


}  // namespace Parser
