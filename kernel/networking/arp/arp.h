
#pragma once

#include "../../shared.h"

// Define the Arp values 
#define ARP_REQUEST 0x0001
#define ARP_REPLY   0x0002


struct ARPHeader {
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

namespace ARP {
    char* encode_arp(uint32_t sender_ip, char* sender_mac, uint32_t target_ip, char* target_mac, uint16_t opcode, uint32_t* packet_length);
    uint32_t decode_arp(ARPHeader* packet, uint32_t* target_ip);
}