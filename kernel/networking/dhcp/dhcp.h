#pragma once

#include "../../shared.h"

// DHCP opcodes
#define DHCP_DISCOVER 0x01
#define DHCP_OFFER 0x02
#define DHCP_REQUEST 0x03
#define DHCP_ACK 0x04

// source: https://dox.ipxe.org/structdhcphdr.html
struct DHCPHeader {
    uint8_t op;            // Operation: 1 for request, 2 for reply
    uint8_t htype;         // Hardware type (1 = Ethernet)
    uint8_t hlen;          // Hardware address length (6 bytes for MAC)
    uint8_t hops;          // Number of hops
    uint32_t xid;          // Transaction ID
    uint16_t secs;         // Seconds elapsed
    uint16_t flags;        // Flags
    uint32_t ciaddr;       // Client IP address
    uint32_t yiaddr;       // Your (client) IP address
    uint32_t siaddr;       // Server IP address
    uint32_t giaddr;       // Gateway IP address
    uint8_t chaddr[16];    // Client hardware address (MAC)
    uint8_t sname[64];     // Server name
    uint8_t file[128];     // Boot file name
    uint32_t magic_cookie; // Magic cookie (fixed value)
    uint8_t options[];     // Options
};

class DHCP {
private: 
    uint32_t ip_addr[10];  // Array of IPs for up to 9 users
    uint32_t curr_index;     // Current index to insert the next IP
    
public:
    DHCP();
    char* encode_dhcp(uint32_t sender_ip, char* sender_mac, uint16_t opcode, uint32_t* packet_length);
    uint32_t decode_dhcp(DHCPHeader* dhcp_head, uint32_t* client_ip);
    void send_dhcp_offer(uint32_t target_ip, char* target_mac);
    void handle_dhcp(DHCPHeader* packet, char* sender_mac);
};