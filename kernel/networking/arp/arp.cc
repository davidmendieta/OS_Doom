#include "../../machine.h"
#include "../../debug.h"
#include "../drivers/rtl8139.h"
#include "../../shared.h"
#include "../protocols/parser.h"
#include "../interface/network_interface.h"
#include "../../heap.h"

#include "promise.h"

extern StrongPtr<NetworkInterface> network_interface;

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
    arp_head->sender_ip = sender_ip; // sender IP address

    memcpy(arp_head->target_mac, target_mac, 6);
    arp_head->target_ip = target_ip;

    *packet_length = sizeof(EthernetHeader) + sizeof(ARPHeader);
    return arp_packet;
}

uint32_t decode_arp(ARPHeader* arp_head, uint32_t* target_ip, uint32_t *sender_ip) {
    Debug::printf("Received ARP packet\n");
    
    if (arp_head->opcode != ARP_REQUEST && arp_head->opcode != ARP_REPLY) { // ARP request
        Debug::printf("Not an ARP request\n");
        return -1;
    }

    *target_ip = arp_head->target_ip;
    *sender_ip = arp_head->sender_ip;

    if (arp_head->opcode == ARP_REPLY){
        Debug::printf("Received ARP reply\n");

        Debug::printf("Sender IP That we got APR NAMESPACE: %0x\n", arp_head->sender_ip);    
        Debug::printf("Sender MAC: %02x:%02x:%02x:%02x:%02x:%02x\n", 
            arp_head->sender_mac[0], arp_head->sender_mac[1], arp_head->sender_mac[2], 
            arp_head->sender_mac[3], arp_head->sender_mac[4], arp_head->sender_mac[5]);
        
        if (network_interface->arp_cache.p[arp_head->sender_ip & 0xFF] == nullptr) {
            Debug::printf("SOMETHING IS WRONG\n");
            network_interface->arp_cache.p[arp_head->sender_ip & 0xFF] = StrongPtr<PromiseWithTimeout>::make(1);
        }
        char* new_mac = (char*)malloc(6);
        memcpy(new_mac, arp_head->sender_mac, 6);
        network_interface->arp_cache.p[arp_head->sender_ip & 0xFF]->set(new_mac);
        return ARP_REPLY; // Magic value 
    }
    
    Debug::printf("Target IP is %08x\n", target_ip);
    return ARP_REQUEST;
}
}