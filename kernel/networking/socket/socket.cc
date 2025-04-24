#include "socket.h"

#include <stdint.h>
#include "../../bb.h"
#include "../../heap.h"
#include "../networking.h"
#include "../protocols/parser.h"



#include "porttable.h"

namespace sockets {

UDPSocket::UDPSocket() {
    recieve_queue = StrongPtr<BB<ReceivedPacket*>>::make(100);
}
UDPSocket::~UDPSocket() {
    if (this->isbound) {
        port_table.remove(this->sockaddr_info.sin_port);
    }
}


void UDPSocket::add(void* buffer, size_t payload_len, uint32_t src_ip, uint16_t src_port) {
    ReceivedPacket* packet = new ReceivedPacket(buffer, payload_len, src_ip, src_port);
    recieve_queue->put(packet);
}

int UDPSocket::bind(const sockaddr_in* addr) {
    if (this->isbound) {
        return -1;
    }
    this->isbound = true;
    int port = ntohs(addr->sin_port);
    if (port == 0) {
        int port = pick_port();
        if (port == 0) {
            return -1;
        }
    }
    this->sockaddr_info.sa_family = addr->sa_family;
    this->sockaddr_info.sin_port = port;
    this->sockaddr_info.sin_addr.s_addr = ntohl(addr->sin_addr.s_addr);
    // Definate race condition exists though I think we will only have one PCB for doom so Ok?
    if (port_table.contains(port)) {
        return -1;
    }
    port_table.bind(addr->sin_port, StrongPtr<UDPSocket>{this});
    return 0;
}


int UDPSocket::getsockname(sockaddr_in* addr) {
    if (!(this->isbound)) {
        return -1;
    }
    Debug::printf("socket name\n");
    addr->sa_family = this->sockaddr_info.sa_family;
    addr->sin_addr.s_addr = htonl(this->sockaddr_info.sin_addr.s_addr);
    addr->sin_port = htons(this->sockaddr_info.sin_port);
    return 0;
}

//Flags not implmeneted...do I need to?
ssize_t UDPSocket::sendto(const void* buf, size_t len, const sockaddr_in* dst) {
    void* send = malloc(len);
    memcpy(send, buf, len);
    uint32_t send_ip = this->sockaddr_info.sin_addr.s_addr;
    if (send_ip == INADDR_ANY) {
        send_ip = network_interface->get_ip();
    }
    if (dst->sin_addr.s_addr == INADDR_ANY || dst->sin_port == 0) {
        return -1;
    }
    size_t out_length = 0;

    
    network_interface->send_ip_udp(send, len, send_ip, this->sockaddr_info.sin_port,
                                ntohl(dst->sin_addr.s_addr), ntohs(dst->sin_port), &out_length);
    free(send);
    return out_length;
}

ssize_t UDPSocket::recvfrom(void* buf, size_t len, sockaddr_in* src, int flags) {
    ReceivedPacket* packet = nullptr;
    if (flags & MSG_PEEK_REV) {
        bool packetExists = recieve_queue->peekNoWait(packet);
        if (!packetExists) {
            if ((flags & MSG_DONTWAIT_REV)) {
                return -1;
            } else {
                packet = recieve_queue->peek();
            }
        }
    } else {
        bool packetExists = recieve_queue->getNoWait(packet);
        if (!packetExists) {
            if ((flags & MSG_DONTWAIT_REV)) {
                return -1;
            } else {
                packet = recieve_queue->get();
            }
        }
    }
    uint32_t copy_length = len <= packet->payload_len? len : packet->payload_len;
    memcpy(buf, packet->buffer, copy_length);
    src->sa_family = AF_INET;
    src->sin_addr.s_addr = htonl(packet->src_ip);
    src->sin_port = htons(packet->src_port);
    return copy_length;

}

}  // namespace sockets