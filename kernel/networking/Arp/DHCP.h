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





