#include "socket.h"

#include <stdint.h>

#include "../../heap.h"
#include "../drivers/rtl8139.h"
#include "../networking.h"
#include "../protocols/parser.h"
#include "porttable.h"

namespace sockets {



UDPSocket::UDPSocket() {}
UDPSocket::~UDPSocket() {
    if (this->isbound) {
        port_table.remove(this->sockaddr_info.sin_port);
    }
}


void UDPSocket::add(void* buffer, size_t payload_len, uint32_t src_ip, uint16_t src_port) {
    RecievedPacket* packet = new RecievedPacket(buffer, payload_len, src_ip, src_port);
    recieve_queue.add(packet);
}

int UDPSocket::bind(const sockaddr_in* addr) {
    if (this->isbound) {
        return -1;
    }
    this->isbound = true;
    int port = addr->sin_port;
    if (port == 0) {
        int port = pick_port();
        if (port == 0) {
            return -1;
        }
    }
    this->sockaddr_info.sa_family = addr->sa_family;
    this->sockaddr_info.sin_port = port;
    this->sockaddr_info.sin_addr.s_addr = addr->sin_addr.s_addr;
    // Definate race condition exists though I think we will only have one PCB for doom so Ok?
    if (port_table.contains(port)) {
        return -1;
    }
    port_table.bind(addr->sin_port, StrongPtr<UDPSocket>{this});
    return 0;
}

ssize_t UDPSocket::sendto(const void* buf, size_t len, const sockaddr_in* dst) {
    void* send = malloc(len);
    memcpy(send, buf, len);
    uint32_t send_ip = this->sockaddr_info.sin_addr.s_addr;
    if (send_ip == INADDR_ANY) {
        send_ip = rtl8139->resolve_ip();
    }
    if (dst->sin_addr.s_addr == INADDR_ANY || dst->sin_port == 0) {
        return -1;
    }
    size_t outlength = 0;
    void* packed = Parser::send(send, len, send_ip, this->sockaddr_info.sin_port,
                                dst->sin_addr.s_addr, dst->sin_port, &outlength);
    free(send);
    rtl8139->send_packet(packed, outlength);
    return outlength;
}

ssize_t UDPSocket::recvfrom(void* buf, size_t len, sockaddr_in* src, int flags) {
    RecievedPacket* packet = recieve_queue.peek();
    if (flags & MSG_PEEK_REV) {
        if (packet == nullptr && (flags & MSG_DONTWAIT_REV)) {
            return -1;
        } else {
            recieve_queue.monitor_add();
            while (packet == nullptr) {
                iAmStuckInALoop(true);
                packet = recieve_queue.peek();
                recieve_queue.monitor_add();
            }
        }
    } else {
        packet = recieve_queue.remove();
        if (packet == nullptr && (flags & MSG_DONTWAIT_REV)) {
            return -1;
        } else {
            recieve_queue.monitor_add();
            while (packet == nullptr) {
                iAmStuckInALoop(true);
                packet = recieve_queue.remove();
                recieve_queue.monitor_add();
            }
        }
    }

    uint32_t copy_length = len <= packet->payload_len? len : packet->payload_len;
    memcpy(buf, packet->buffer, copy_length);
    src->sin_addr.s_addr = packet->src_ip;
    src->sin_port = packet->src_port;
    if (!(flags & MSG_PEEK_REV)) {
        free(packet);
    }
    return copy_length;
}

}  // namespace sockets