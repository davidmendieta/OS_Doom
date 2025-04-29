
#include <stddef.h>
#include <stdint.h>

#include "network_interface.h"
#include "../drivers/rtl8139.h"
#include "../protocols/parser.h"
#include "../arp/arp.h"
#include "../socket/socket.h"
#include "../socket/porttable.h"
#include "../protocols/tcp.h"

void NetworkInterface::send(const char* payload, uint32_t length) {
    rtl8139.send_packet(payload, length);
}

void NetworkInterface::handle_arp(ARPHeader* packet, char* sender_mac) {
    uint32_t target_ip;
    uint32_t sender_ip;
    uint32_t decode = ARP::decode_arp(packet, &target_ip, &sender_ip);
    
    if(decode == ARP_REPLY) {
        
        return;
    }


    if (target_ip != my_IP) { // not for us
        Debug::printf("Not for us my the target_ip = %x my IP is %x\n", target_ip, my_IP);
        return;
    }

    Debug::printf("ARP request for us\n");

    // Send ARP reply
    char* my_mac = (char*)malloc(6);;
    rtl8139.get_mac_addr(my_mac);
    
    uint32_t packet_length;
    void* payload = ARP::encode_arp(my_IP, my_mac, sender_ip, sender_mac, ARP_REPLY, &packet_length); // ARP reply
    rtl8139.send_packet(payload, packet_length);
}

void NetworkInterface::handle_ip_udp(IPv4Header* packet, uint32_t packet_length) {
        
    size_t payload_length;
    uint32_t src_ip;
    uint16_t src_port;
    uint16_t dst_port;

    if (packet->protocol == 17) {
        char* received_packet = Parser::decode_ip_udp(
            packet, 
            packet_length, 
            &payload_length, 
            &src_ip, 
            &src_port, 
            &dst_port
        );
        if(received_packet == nullptr) {
            //Debug::printf("ERROR HANDLING IP UDP\n");
            return;
        }
    
        Debug::printf("RECEIVED PACKET\n");
        for(uint32_t i = 0; i < payload_length; i++) {
            Debug::printf("%x", received_packet[i]);
        }
        Debug::printf("\n");

        StrongPtr<sockets::UDPSocket> socket =
            sockets::port_table.get(dst_port);
        if (socket != nullptr) {
            socket->add(received_packet, payload_length, src_ip, src_port);
        }
    } else if (packet->protocol == 6){
        uint8_t* packet_start = (uint8_t*)packet;
        uint8_t ihl_bytes = (packet->version_ihl & 0x0F) * 4;

        TCPHeader* tcp_hdr = (TCPHeader*)(packet_start + ihl_bytes);

        Parser::receive_tcp(packet, tcp_hdr, packet_length - ihl_bytes);    
    }
}
