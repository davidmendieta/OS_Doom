// #include "DHCP.h"
// #include "drivers/rtl8139.h"
// #include "debug.h"


// #define ETH_TYPE_IP  0x0800
// #define IP_PROTO_UDP 0x11
// #define UDP_PORT_DHCP_SERVER 67
// #define UDP_PORT_DHCP_CLIENT 68
// #define DHCP_MAGIC 0x63825363

// /* DHCP option codes */
// #define DHCP_OPT_MSGTYPE  53
// #define DHCP_DISCOVER     1
// #define DHCP_OFFER        2
// #define DHCP_REQUEST      3
// #define DHCP_ACK          5
// #define DHCP_OPT_END     255




// struct Header_data
// {
//     /* data */
//     uint8_t op; // 1 byte
// };


// enum DeviceType {
//     DHCP_SERVER,
//     DHCP_CLIENT

// };
// DeviceType device_type = DHCP_CLIENT;

// void DHCP_server(){

//     device_type = DHCP_SERVER;


//     while (1){
//         // wait for a packet
//         // get the packet
//         // check if its a dhcp packet
//         // check if its a discover packet
//         // send an offer packet
//         // wait for a request packet
//         // send an ack packet

        
//     }
// }
// void return_ip(char* packet , int packet_len){
//     // check if its a dhcp packet
//     // check if its a discover packet
//     // send an offer packet
//     // wait for a request packet
//     // send an ack packet

//     // 
//     if (((int32_t*)packet)[32] == 1){ // ITS An arp
//         Debug::printf("DHCP discover packet\n");
//         packet = send_wide_broadcast(packet); // this is where we send the wide broadcast 
//         // 

//         // send_wide_broadcast(char* packet) {
//         //     bool found_dhcp = false;
//         //.    while (!found_dhcp) {
//         //         keep on looking at devices and checking their unique id until it matches dhcp
//         //         found_dhcp = true;
//         //.    }
//         //.    return dhcp(packet);
//         // }
//         //
//         // dhcp(char* packet) {
//         //.    if (client) // client should be 68 {
//         //.         extract the ip address
//         //          do a "check" on the ip address to make sure its valid
//         //          if its valid then create a new packet, stuff the new ipv4 address (ret_packet)
//         //          return ret_packet;
//         //.    }
//         //.    else {
//         //          panic
//         //     }

//         //.}

//     } else {
//         Debug::printf("Not a dhcp discover packet\n");
//         return;

//     }
    
// }

// right now we are in the "device"
// automatically generates random ipv4
// stuffs it in the packet
// send it to dhcp
// dhcp gets the packet
// just gives a thumbs up
// stuff that same ipv4 given intially as both the given and returned ipv4

// my_ipv4 = whatever it gave me
// ret_packet = dhcp(packet))
// traverse through the packet to get the ipv4