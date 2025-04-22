// // Function just to Hand out IP addresses


// // DHCP server



// #pragma once
// #include "drivers/rtl8139.h"
// #include "../shared.h"
// #include "protocols/parser.h"


// struct device
// {
//     /* data */
//     // unique number
//     // 
//     uint32_t mac;
//     uint32_t ip;
     
// };

// // dhcp server info
// struct DHCP
// {
//     /* data */
//     uint16_t amount_of_devices;
//     uint16_t max_devices;
//     uint32_t starting_ip;
//     device* devices;
// };




// // we need something to identify the dhcp when the computer does the wide broadcast
// // 32 bit number
// // 192.168.100.1 <- starting address
// // no hash table, just increment by 1

// // its in a while loop until it recieves a message from a device


// void dhcp_server();

// void return_ip(char* packet , uint32_t packet_len);





#pragma once

#include "debug.h"
#include "../../shared.h" 

struct EthernetHeader_arp
{
    uint8_t dest_mac[6]; 
    uint8_t src_mac[6]; 
    uint16_t ethertype; 
};

struct arp_header {
    uint16_t hardware_type;
    uint16_t protocol_type;
    uint8_t  hardware_size; 
    uint8_t  protocol_size; 
    uint16_t opcode;
    uint8_t  sender_mac[6];
    uint32_t sender_ip;
    uint8_t  target_mac[6];
    uint32_t target_ip;
};

void dhcp_init(uint32_t start_ip);

void receive_dhcp(uint8_t* packet, int len);