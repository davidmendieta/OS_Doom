#include "tcp.h"
#include "parser.h"
#include "../../heap.h"
#include "../../machine.h"
#include "../interface/network_interface.h"
#include "../networking.h"


void send_tcp_syn(TCB_tcp* conn) {
    TCPHeader hdr = {};
    hdr.src_port = htons(conn->local_port);
    hdr.dest_port = htons(conn->remote_port);
    hdr.seq_num = htonl(conn->seq_num);
    hdr.ack_num = 0;
    hdr.data_offset_reserved = (sizeof(TCPHeader) / 4) << 4; // header size
    hdr.flags = 0x02; // SYN flag
    hdr.window_size = htons(1024); 
    hdr.checksum = 0;

    // No payload for SYN
    send_tcp_packet(conn->remote_ip, &hdr, nullptr, 0);
}

void send_tcp_syn_ack(TCB_tcp* conn) {
    TCPHeader hdr = {};
    hdr.src_port = htons(conn->local_port);
    hdr.dest_port = htons(conn->remote_port);
    hdr.seq_num = htonl(conn->seq_num);
    hdr.ack_num = htonl(conn->ack_num);
    hdr.data_offset_reserved = (sizeof(TCPHeader) / 4) << 4;
    hdr.flags = 0x12; // SYN + ACK
    hdr.window_size = htons(1024);
    hdr.checksum = 0;

    send_tcp_packet(conn->remote_ip, &hdr, nullptr, 0);
}

void send_tcp_ack(TCB_tcp* conn) {
    TCPHeader hdr = {};
    hdr.src_port = htons(conn->local_port);
    hdr.dest_port = htons(conn->remote_port);
    hdr.seq_num = htonl(conn->seq_num);
    hdr.ack_num = htonl(conn->ack_num);
    hdr.data_offset_reserved = (sizeof(TCPHeader) / 4) << 4;
    hdr.flags = 0x10; // ACK flag
    hdr.window_size = htons(1024);
    hdr.checksum = 0;

    send_tcp_packet(conn->remote_ip, &hdr, nullptr, 0);
}

extern NetworkInterface* network_interface

void send_tcp_packet(uint32_t dst_ip, const TCPHeader* tcp_hdr, const uint8_t* payload, size_t payload_len) {
    size_t tcp_len = sizeof(TCPHeader) + payload_len;

    uint8_t* buf = (uint8_t*)malloc(tcp_len);
    memcpy(buf, tcp_hdr, sizeof(TCPHeader));
    if (payload && payload_len) {
        memcpy(buf + sizeof(TCPHeader), payload, payload_len);
    }

    uint32_t packet_len;
    char* packet = Parser::encode_ip_tcp(
        buf, tcp_len,
        network_interface->get_ip(),
        nullptr,
        ntohs(tcp_hdr->src_port),    
        dst_ip,
        nullptr,               
        ntohs(tcp_hdr->dest_port),
        &packet_len
    );

    // MAC addresses 
    network_interface->send(packet, packet_len);

    free(buf);
    free(packet);
}

