#include "../../machine.h"
#include "../../debug.h"
#include "../drivers/rtl8139.h"
#include "../../shared.h"
#include "../protocols/parser.h"
#include "../interface/network_interface.h"
#include "../../heap.h"

#include "arp.h"

namespace ARP {

char* encode_arp(uint32_t sender_ip, char* sender_mac, uint32_t target_ip, char* target_mac, uint16_t opcode, uint32_t* packet_length) {
    char* arp_packet = (char*)malloc(sizeof(EthernetHeader) + sizeof(ARPHeader));

    EthernetHeader* ethernet_header = (EthernetHeader*)arp_packet;

    // Ethernet Header
    memcpy(ethernet_header->src_mac, sender_mac, 6);
    memcpy(ethernet_header->dest_mac, target_mac, 6);
    ethernet_header->ethertype = ARP_VALUE;

    // ARP Header
    ARPHeader* arp_head = (ARPHeader*)(arp_packet + sizeof(EthernetHeader));
    arp_head->hardware_type = 0x1; // hardware type: Ethernet
    arp_head->protocol_type = IPV4_VALUE; // protocol type: IPv4
    arp_head->hardware_size = 0x06; // hardware size: MAC address length
    arp_head->protocol_size = 0x04; // protocol size: IPv4 address length
    arp_head->opcode = opcode; // opcode: request or reply
    
    memcpy(arp_head->sender_mac, sender_mac, 6);
    arp_head->sender_ip = *(uint32_t*)sender_ip; // sender IP address

    memcpy(arp_head->target_mac, target_mac, 6);
    arp_head->target_ip = target_ip;

    *packet_length = sizeof(arp_packet);
    return arp_packet;
}

uint32_t decode_arp(ARPHeader* arp_head, uint32_t* target_ip){
    Debug::printf("Received ARP packet\n");

    if (arp_head->opcode != ARP_REQUEST) { // ARP request
        Debug::printf("Not an ARP request\n");
        return -1;
    }
    
    *target_ip = arp_head->target_ip;
    Debug::printf("Target IP is %08x\n", target_ip);
    return 0;
}

}