#include "../../machine.h"
#include "../../debug.h"
#include "../drivers/rtl8139.h"
#include "../../shared.h"
#include "../protocols/parser.h"
#include "../interface/network_interface.h"
#include "../../heap.h"

#include "dhcp.h"

StrongPtr<RTL8139> rtl8139;

DHCP::DHCP() {
    curr_index = 0;
    // Initialize IP addresses to 0 
    for (int i = 0; i < 10; i++) {
        ip_addr[i] = 0;
    }
    rtl8139 = new RTL8139();
}

char* DHCP::encode_dhcp(uint32_t sender_ip, char* sender_mac, uint16_t opcode, uint32_t* packet_length) {
    char* dhcp_packet = (char* ) malloc(sizeof(EthernetHeader) + sizeof(DHCPHeader));
    EthernetHeader* ethernet_header = (EthernetHeader* ) dhcp_packet;

    // Ethernet header
    memcpy(ethernet_header->src_mac, sender_mac, 6);
    memcpy(ethernet_header->dest_mac, (char* )"\xFF\xFF\xFF\xFF\xFF\xFF", 6);
    
    // TODO: Fix this
    ethernet_header->ethertype = IPV4_VALUE; // make it DHCP_VALUE ?? 

    // DHCP header
    // source: https://dox.ipxe.org/structdhcphdr.html
    DHCPHeader* dhcp_head = (DHCPHeader* ) (dhcp_packet + sizeof(EthernetHeader));
    dhcp_head->op = opcode;                   // DHCP operation (discover, offer, request, ack)
    dhcp_head->htype = 0x01;                  // Ethernet
    dhcp_head->hlen = 0x06;                   // MAC address length (6 bytes)
    dhcp_head->hops = 0x00;                   // Number of hops
    dhcp_head->xid = 0x12345678;              // Transaction ID (idk i just gave a random one)
    dhcp_head->secs = 0;                      // Seconds elapsed
    dhcp_head->flags = 0x0000;                // Flags
    dhcp_head->ciaddr = 0;                    // Client IP address
    dhcp_head->yiaddr = sender_ip;            // Your IP address (for OFFER/ACK)
    dhcp_head->siaddr = 0;                    // Server IP address
    dhcp_head->giaddr = 0;                    // Gateway IP address
    memcpy(dhcp_head->chaddr, sender_mac, 6); // Client MAC address
    dhcp_head->magic_cookie = 0x63825363;     // Magic cookie (standard value, google search)
 
    *packet_length = sizeof(EthernetHeader) + sizeof(DHCPHeader);
    
    return dhcp_packet;
}

// 0 = good, -1 = bad
uint32_t DHCP::decode_dhcp(DHCPHeader* dhcp_head, uint32_t* client_ip) {
    if (dhcp_head->op == DHCP_REQUEST) {
        *client_ip = dhcp_head->yiaddr;
        return 0;
    }
    return -1;
}

void DHCP::send_dhcp_offer(uint32_t target_ip, char* target_mac) {
    uint32_t packet_length;
    void* packet = encode_dhcp(target_ip, target_mac, DHCP_OFFER, &packet_length);
    rtl8139->send_packet(packet, packet_length);
}

void DHCP::handle_dhcp(DHCPHeader* packet, char* sender_mac) {
    uint32_t client_ip = 0;
    uint32_t result = decode_dhcp(packet, &client_ip);

    if (result < 0) {
        Debug::printf("Invalid DHCP packet\n");
        return;
    }

    if (client_ip == 0) {
        Debug::printf("Client IP not valid\n");
        return;
    }

    ip_addr[curr_index] = client_ip;

    curr_index += 1;

    send_dhcp_offer(client_ip, sender_mac);
}
