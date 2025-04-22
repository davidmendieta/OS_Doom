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



// #include "DHCP.h"

// extern StrongPtr<RTL8139> rtl8139;

// const uint32_t MAX_LEASES = 10;

// uint32_t leases[MAX_LEASES];
// uint32_t pool_start;
// uint8_t  next_index;

// void dhcp_init(uint32_t start_ip) {
//     pool_start = start_ip;
//     next_index = 0;
//     for (uint8_t i = 0; i < MAX_LEASES; i++) {
//         leases[i] = 0;
//     }
// }

// void convert_ip_to_bytes(uint32_t ip, uint8_t out[4]) {
//     out[0] = (ip >> 24) & 0xFF;
//     out[1] = (ip >> 16) & 0xFF;
//     out[2] = (ip >>  8) & 0xFF;
//     out[3] = ip & 0xFF;
// }

// void send_dhcp_packet(const uint8_t client_mac[8], const uint8_t client_ip[4], uint32_t assigned_ip) {
//     if (rtl8139 == nullptr) {
//         Debug::panic("RTL8139 not initialized");
//     }

//     uint8_t arp_packet[42] __attribute__((aligned(16))) = {0};

//     for (int i = 0; i < 6; i++) {
//         arp_packet[i] = client_mac[i];
//     }

//     for (int i = 0; i < 6; i++) {
//         arp_packet[6 + i] = rtl8139->mac_addr[i];
//     }

//     arp_packet[12] = 0x08;
//     arp_packet[13] = 0x06;

//     // arp payload
//     arp_packet[14] = 0x00;
//     arp_packet[15] = 0x01; // hardware_type

//     arp_packet[16] = 0x08;
//     arp_packet[17] = 0x00; // protocol_type

//     arp_packet[18] = 6;    // hardware_size
//     arp_packet[19] = 4;    // protocol_size

//     arp_packet[20] = 0x00;
//     arp_packet[21] = 0x02; // opcode = reply

//     for (int i = 0; i < 6; i++) {
//         arp_packet[22 + i] = rtl8139->mac_addr[i];
//     }

//     // assigned_ip to bytes (big endian)
//     uint8_t asn[4];
//     convert_ip_to_bytes(assigned_ip, asn);

//     for (int i = 0; i < 4; i++) {
//         arp_packet[28 + i] = asn[i];
//     }

//     for (int i = 0; i < 6; i++) {
//         arp_packet[32 + i] = client_mac[i];
//     }

//     for (int i = 0; i < 4; i++) {
//         arp_packet[38 + i] = client_ip[i];
//     }

//     rtl8139->send_packet(arp_packet, sizeof(arp_packet));
// }

// void recieve_dhcp_packet(uint8_t* packet, int len) {
//     // sanity check
//     if (len < 42) {
//         return;
//     }

//     EthernetHeader_arp* eth_header = (EthernetHeader_arp*)packet;
//     arp_header* arp_head = (arp_header*)(packet + sizeof(EthernetHeader_arp));

//     // get client mac and ip bytes
//     uint8_t client_mac[6];
//     for (int i = 0; i < 6; i++) {
//         client_mac[i] = arp_head->sender_mac[i];
//     }

//     uint8_t client_ip[4];

//     // sip = sender_ip
//     uint8_t* sip = (uint8_t*) &arp_head->sender_ip;
//     for (int i = 0; i < 4; i++) {
//         client_ip[i] = sip[i];
//     }

//     // get the next ip from our pool, we should not really even need the wrap, i just have to have 
//     uint32_t assigned = pool_start + next_index;
//     leases[next_index] = assigned;

//     // wrap, we should not really 
//     next_index = (next_index + 1) % MAX_LEASES;

//     send_dhcp_packet(client_mac, client_ip, assigned);
// }