#include "../drivers/rtl8139.h"

enum EtherType { ARP = 0x0806, IPV4 = 0x0800 };

class NetworkInterface {
    uint32_t my_IP;
    RTL8139 rtl8139{};

   public:
    uint32_t get_io_base() { return rtl8139.io_base; }

    uint32_t get_ip() { return my_IP; }

    void send(const char* payload, uint32_t length) {
        rtl8139.send_packet(payload, length);
    }

    void receive() {
        uint32_t packet_length;
        EthernetHeader* packet =
            (EthernetHeader*)rtl8139.get_packet(&packet_length);

        uint32_t ethertype = packet->ethertype;

        switch (ethertype) {
            case ARP:
                break;
            case IPV4:
                handle_ip_udp(
                    (IPv4Header*)((void*)packet + sizeof(EthernetHeader)),
                    packet_length);
                break;
            default:
                Debug::printf("ETHERTYPE NOT RECOGNIZED");
                return;
        }
    }

    void handle_ip_udp(IPv4Header* packet, uint32_t packet_length) {
        size_t payload_length = 0;
        uint32_t src_ip = 0;
        uint16_t src_port = 0;
        uint16_t dst_port = 0;
        void* received_packet =
            Parser::decode_ip_udp(packet, packet_length, &payload_length,
                                  &src_ip, &src_port, &dst_port);
        if (!received_packet) {
            StrongPtr<sockets::UDPSocket> socket =
                sockets::port_table.get(dst_port);
            if (socket != nullptr) {
                socket->add(received_packet, payload_length, src_ip, src_port);
            }
        }
    }
};