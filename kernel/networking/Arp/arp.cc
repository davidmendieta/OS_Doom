#include "../../machine.h"
#include "../../debug.h"
#include "../drivers/rtl8139.h"
#include "../../shared.h"

struct EthernetHeader_arp
{
    uint8_t dest_mac[6]; // destination MAC address
    uint8_t src_mac[6]; // source MAC address
    uint16_t ethertype; // Ethertype (0x0800 for IPv4)
};




struct arp_header
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





void send_arp(uint8_t* ip, uint8_t opcode) {
    if (rtl8139 == nullptr) {
        Debug::panic("RTL8139 not initialized");
    }
    // uint8_t arp_packet[60] __attribute__((aligned(16))) = { // We want anyone with this mac adress to respond
    //     0xff,0xff,0xff,0xff,0xff,0xff, // broadcast dest MAC
    //     0x12,0x34,0x56,0x78,0x9a,0xbc, // source MAC
    //     0x08,0x00,                     // Ethertype: IPv4
    //     'h','e','l','l','o'
    // };

    // The ip we want

    uint8_t my_ip[4] = {192, 168, 1, 1};

    Debug::printf("The size of the arp is %d\n", sizeof(arp_header));

     // we just default send this, dhcp will fix it 
    // uint8_t mac[6] = {0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc}; // our mac address

    uint8_t arp_packet[42] __attribute__((aligned(16))) = {0}; // size of ARP header

    // Fill the ethernet Header
    arp_packet[0] = 0xFF; arp_packet[1] = 0xFF; 
    arp_packet[2] = 0xFF; arp_packet[3] = 0xFF;
    arp_packet[4] = 0xFF; arp_packet[5] = 0xFF; // broadcast dest MAC

    for (int i = 0; i < 6; i++) {
        arp_packet[6 + i] = rtl8139->mac_addr[i]; // source MAC address
    }
    arp_packet[12] = 0x08; arp_packet[13] = 0x06; // Ethertype: ARP



    // this is an arp payload
    arp_packet[14] = 0x00; arp_packet[15] = 0x01; // hardware type: Ethernet
    arp_packet[16] = 0x08; arp_packet[17] = 0x00; // protocol type: IPv4
    arp_packet[18] = 0x06;                       // hardware size
    arp_packet[19] = 0x04;                       // protocol size
    arp_packet[20] = 0x00; arp_packet[21] = opcode; // opcode: request or reply

    for (int i = 0; i < 6; i++) {
        arp_packet[22 + i] = rtl8139->mac_addr[i]; // sender MAC address
    }
    for (int i = 0; i < 4; i++) {
        arp_packet[28 + i] = my_ip[i]; // sender IP address
    }
    for (int i = 0; i < 6; i++) {
        arp_packet[32 + i] = 0x00; // target MAC address
    }
    for (int i = 0; i < 4; i++) {
        arp_packet[38 + i] = ip[i]; // target IP address
    }
    // Now lets send shit
    
    // uint32_t header_size = sizeof(arp_header);

    rtl8139->send_packet(arp_packet, sizeof(arp_packet)); // by default, send packet is a wide broadcast
    Debug::printf("Sent ARP packet\n");
}

// For now Global variable
uint32_t my_ip_adr = 0; //
unsigned int my_ip() {
    if (my_ip_adr == 0) {
        // Find MY IP trough protocal or set it to deafault 192.72.123;
        // port 67 = DHCP
        // port 68 = user
    }

    return 0; // NOT IMPLEMENTED
}

void receive_arp(uint8_t* packet, int len){


    Debug::printf("Received ARP packet\n");

    len = 42; // for now, we assume the packet is 42 bytes long
    for (int i = 0; i < len; i++) {
        Debug::printf("%02x ", packet[i]);
    }
    Debug::printf("\n");

    EthernetHeader_arp* eth_header = (EthernetHeader_arp*)packet;
    if (eth_header->ethertype != 0x0806) { 
        Debug::printf("Not an ARP packet\n");
        // return;
    }
    Debug::printf("the sixe of the packet is %d\n", len);
    Debug::printf("the size of the ethernet header is %d\n", sizeof(EthernetHeader_arp));
    arp_header* arp_head = (arp_header*)(packet + sizeof(EthernetHeader_arp));
    
    if (arp_head->opcode != 0x0001) { // ARP request
        Debug::printf("Not an ARP request\n");
        // add 
        return;
    }
    if (arp_head->target_ip != my_ip()) { // not for us
        Debug::printf("Not for us\n");
        return;
    } else {
        Debug::printf("ARP request for us\n");
        // Send ARP reply
        send_arp(, 0x0002); // ARP reply
        
    }
    
}
