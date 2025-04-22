
#include <stddef.h>
#include <stdint.h>

#include "network_interface.h"
#include "../drivers/rtl8139.h"
#include "../protocols/parser.h"
#include "../arp/arp.h"
#include "../socket/socket.h"
#include "../socket/porttable.h"

void NetworkInterface::send(const char* payload, uint32_t length) {
    rtl8139.send_packet(payload, length);
}

void NetworkInterface::handle_arp(ARPHeader* packet, char* sender_mac) {
    uint32_t target_ip;
    uint32_t decode = ARP::decode_arp(packet, &target_ip);
    
    if(decode < 0) {
        return;
    }

    if (target_ip != my_IP) { // not for us
        Debug::printf("Not for us\n");
        return;
    }

    Debug::printf("ARP request for us\n");

    // Send ARP reply
    char* my_mac = (char*)malloc(6);;
    rtl8139.get_mac_addr(my_mac);
    
    uint32_t packet_length;
    void* payload = ARP::encode_arp(my_IP, my_mac, target_ip, sender_mac, ARP_REPLY, &packet_length); // ARP reply
    rtl8139.send_packet(payload, packet_length);
}

void NetworkInterface::handle_ip_udp(IPv4Header* packet, uint32_t packet_length) {
        
    size_t payload_length;
    uint32_t src_ip;
    uint16_t src_port;
    uint16_t dst_port;

    void* received_packet = Parser::decode_ip_udp(
        packet, 
        packet_length, 
        &payload_length, 
        &src_ip, 
        &src_port, 
        &dst_port
    );

    if(received_packet == nullptr) {
        Debug::printf("ERROR HANDLING IP UDP\n");
        return;
    }

    StrongPtr<sockets::UDPSocket> socket =
        sockets::port_table.get(dst_port);
    if (socket != nullptr) {
        socket->add(received_packet, payload_length, src_ip, src_port);
    }
}