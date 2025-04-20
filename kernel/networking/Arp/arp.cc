#include "../../machine.h"
#include "../../debug.h"
#include "../drivers/rtl8139.h"
#include "../../shared.h"


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

void send_arp(uint8_t* ip, uint8_t* mac) {
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

    Debug::printf("The size of the arp is %d\n", sizeof(arp_header));

    uint8_t my_ip[4] = {192, 168, 1, 1};
    // uint8_t mac[6] = {0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc}; // our mac address

    uint8_t arp_packet[42] __attribute__((aligned(16))) = {0}; // size of ARP header

    // Fill static fields
    arp_packet[0] = 0x00; arp_packet[1] = 0x01; // hardware type: Ethernet
    arp_packet[2] = 0x08; arp_packet[3] = 0x00; // protocol type: IPv4
    arp_packet[4] = 0x06;                       // hardware size
    arp_packet[5] = 0x04;                       // protocol size
    arp_packet[6] = 0x00; arp_packet[7] = 0x01; // opcode: request

    for (int i = 0; i < 6; i++) {
        arp_packet[8 + i] = mac[i]; // sender MAC address
    }
    for (int i = 0; i < 4; i++) {
        arp_packet[14 + i] = my_ip[i]; // sender IP address
    }
    for (int i = 0; i < 6; i++) {
        arp_packet[18 + i] = 0x00; // target MAC address
    }
    for (int i = 0; i < 4; i++) {
        arp_packet[24 + i] = ip[i]; // target IP address
    }
    // Now lets send shit
    
    // uint32_t header_size = sizeof(arp_header);

    





    


    rtl8139->send_packet(arp_packet, sizeof(arp_packet));
    Debug::printf("Sent ARP packet\n");
}

