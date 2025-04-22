#include "../../machine.h"
#include "../../debug.h"
#include "../drivers/rtl8139.h"
#include "../../shared.h"
#include "../protocols/parser.h"

#include "arp.h"

struct ArpHeader
{
    uint16_t hardware_type; // 1 for ethernet
    uint16_t protocol_type; // 0x0800 for ipv4
    uint8_t hardware_size; // 6 for ethernet
    uint8_t protocol_size; // 4 for ipv4
    uint16_t opcode; // 1 for request, 2 for reply
    uint8_t sender_mac[6]; // sender mac address
    uint32_t sender_ip; // sender ip address
    uint8_t target_mac[6]; // target mac address
    uint32_t target_ip; // target ip address
};

extern StrongPtr<RTL8139> rtl8139;

void send_arp(uint32_t target_ip, uint8_t opcode) {
    if (rtl8139 == nullptr) {
        Debug::panic("RTL8139 not initialized");
    }
    uint8_t my_ip[4] = {192, 168, 1, 1};
    uint8_t arp_packet[46] __attribute__((aligned(16))) = {0}; // size of ARP header

    // Fill the ethernet Header
    arp_packet[0] = 0xFF; arp_packet[1] = 0xFF; 
    arp_packet[2] = 0xFF; arp_packet[3] = 0xFF;
    arp_packet[4] = 0xFF; arp_packet[5] = 0xFF; // broadcast dest MAC

    for (int i = 0; i < 6; i++) {
        arp_packet[6 + i] = rtl8139->mac_addr[i]; // source MAC address
    }
    ((EthernetHeader*)arp_packet)->ethertype = ARP_VALUE; // Ethertype: ARP

    ArpHeader* arp_head = (ArpHeader*)(arp_packet + sizeof(EthernetHeader));
    arp_head->hardware_type = 0x1; // hardware type: Ethernet
    arp_head->protocol_type = 0x0800; // protocol type: IPv4
    arp_head->hardware_size = 0x06; // hardware size: MAC address length
    arp_head->protocol_size = 0x04; // protocol size: IPv4 address length
    arp_head->opcode = opcode; // opcode: request or reply

    
    for (int i = 0; i < 6; i++) {
        arp_head->sender_mac[i] = rtl8139->mac_addr[i]; // sender MAC address
    }
    arp_head->sender_ip = *(uint32_t*)my_ip; // sender IP address

    for (int i = 0; i < 6; i++) {
        arp_head->target_mac[i] = 0x00; // target MAC address
    }
    
    arp_head->target_ip = target_ip;

    rtl8139->send_packet(arp_packet, sizeof(arp_packet)); // by default, send packet is a wide broadcast
    Debug::printf("Sent ARP packet\n");
}


// For now Global variable
uint32_t my_ip_adr = 0; //
uint32_t my_ip() {
    if (my_ip_adr == 0) {
        // Find MY IP trough protocal or set it to deafault 192.72.123;
        // port 67 = DHCP
        // port 68 = user
    }
    return 0; // NOT IMPLEMENTED
}

void receive_arp(uint8_t* packet, int len){
    Debug::printf("Received ARP packet\n");
    EthernetHeader* eth_header = (EthernetHeader*)packet;
    uint16_t ether_type = eth_header->ethertype;

    if (ether_type != ARP_VALUE) { // little endian 
        Debug::printf("Not an ARP packet\n");
        Debug::printf("Ethertype is in thee arp resive is  %x\n", ether_type);
        return;
    }
    Debug::printf("the size of the packet is %d\n", len);
    Debug::printf("the size of the ethernet header is %d\n", sizeof(EthernetHeader));
    ArpHeader* arp_head = (ArpHeader*)(packet + sizeof(EthernetHeader));

    for (int i = 0; i < len - (int)sizeof(EthernetHeader); i++) {
        Debug::printf("%x ", ((uint8_t*)arp_head)[i]);
    }
    
    Debug::printf("Arp code is %x\n", arp_head->opcode);

    if (arp_head->opcode != ARP_REQUEST) { // ARP request
        Debug::printf("Not an ARP request\n");
        return;
    }
    uint32_t target_ip = arp_head->target_ip;
    Debug::printf("Target IP is %08x\n", target_ip);

    if (target_ip != rtl8139->my_ip) { // not for us
        Debug::printf("Not for us\n");
        Debug::printf("Sender IP is %x\n", target_ip);
        Debug::printf("Our IP is %x\n", rtl8139->my_ip);
        return;
    } else {
        Debug::printf("ARP request for us\n");
        // Send ARP reply
        send_arp(arp_head->target_ip, ARP_REPLY); // ARP reply
    }
}
