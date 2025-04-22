
#pragma once
// Define the Arp values 
#define ARP_REQUEST 0x0001
#define ARP_REPLY   0x0002
#define ARP_VALUE 0x0806
#define IPV4_VALUE 0x0800


#include "../../shared.h"
extern void send_arp(uint8_t* ip, uint8_t opcode);


extern void receive_arp(uint8_t* packet, int len);