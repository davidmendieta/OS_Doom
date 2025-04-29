#pragma once
#include <stdint.h>

struct TCPHeader {
    uint16_t src_port;
    uint16_t dest_port;
    uint32_t seq_num;
    uint32_t ack_num;
    uint8_t  data_offset_reserved; // high 4 bits: header size
    uint8_t  flags;                // SYN, ACK, FIN, etc.
    uint16_t window_size;
    uint16_t checksum;
    uint16_t urgent_pointer;
} __attribute__((packed));

enum class TCPState {
    CLOSED,
    LISTEN,
    SYN_SENT,
    SYN_RECEIVED,
    ESTABLISHED,
    FIN_WAIT_1,
    FIN_WAIT_2,
    TIME_WAIT,
    CLOSING
};

struct TCB_tcp {
    TCPState state;
    uint32_t seq_num;
    uint32_t ack_num;
    uint16_t local_port;
    uint16_t remote_port;
    uint32_t remote_ip;
};


void send_tcp_syn(TCB_tcp* conn);
void send_tcp_syn_ack(TCB_tcp* conn);
void send_tcp_ack(TCB_tcp* conn);
void send_tcp_packet(uint32_t dst_ip, const TCPHeader* tcp_hdr, const uint8_t* payload, uint32_t payload_len);