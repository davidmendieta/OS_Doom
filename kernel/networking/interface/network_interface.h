#pragma once

#include "../drivers/rtl8139.h"
#include "../protocols/parser.h"
#include "../arp/arp.h"
#include "../../heap.h"

enum EtherType { ARP_VALUE = 0x0806, IPV4_VALUE = 0x0800, };

class NetworkInterface {
    uint32_t my_IP;
    RTL8139 rtl8139{};

    void handle_arp(ARPHeader* packet, char* sender_mac);
    void handle_ip_udp(IPv4Header* packet, uint32_t packet_length);
    
    public:

    NetworkInterface(uint32_t ip): my_IP(ip){};

    void send(const char* payload, uint32_t length);

    uint32_t get_io_base() { return rtl8139.io_base; }

    uint32_t get_ip() { return my_IP; }

    void send_ip_udp(const void* payload, size_t payload_length, uint32_t src_ip, uint16_t src_port, uint32_t dest_ip, uint16_t dest_port, size_t* out_length) {
        char* src_mac = (char*)malloc(6);
        rtl8139.get_mac_addr(src_mac);
        unsigned char dest_mac[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff}; //! GET FROM HASHMAP / ARP REQUEST IF NOT
        uint32_t packet_length;
        char* packet = Parser::encode_ip_udp(payload, payload_length, src_ip, src_mac, src_port, dest_ip, (char*)dest_mac, dest_port, &packet_length);
        send(packet, packet_length);
    }

    void send_arp_request(uint32_t target_ip) {
        char* my_mac = (char*)malloc(6);
        rtl8139.get_mac_addr(my_mac);
        char target_mac[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        uint32_t packet_length;
        char* packet = ARP::encode_arp(my_IP, my_mac, target_ip, target_mac, ARP_REQUEST, &packet_length);

        Debug::printf("Sending ARP request to %x\n", target_ip);
        for(uint32_t i = 0; i < packet_length; i++) {
            Debug::printf("%x ", packet[i]);
        }
        Debug::printf("\n");

        send(packet, packet_length);
    }

    void receive() {
        uint32_t packet_length;
        EthernetHeader* packet = (EthernetHeader*)rtl8139.get_packet(&packet_length);

        uint32_t ethertype = packet->ethertype;
        char* sender_mac = packet->src_mac;

        switch (ethertype) {
            case ARP_VALUE:
                handle_arp((ARPHeader*)((char*)packet + sizeof(EthernetHeader)), sender_mac);
                break;
            case IPV4_VALUE:
                handle_ip_udp((IPv4Header*)((char*)packet + sizeof(EthernetHeader)), packet_length - sizeof(EthernetHeader));
                break;
            default:
                Debug::printf("ETHERTYPE NOT RECOGNIZED\n");
                return;
        }
    }
};