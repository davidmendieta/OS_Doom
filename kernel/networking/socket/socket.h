#pragma once
#include <stddef.h>
#include <stdint.h>

#include "../../shared.h"

template<typename T>
class BB;   

namespace sockets {

#define AF_UNSPEC 0
#define AF_INET 2

#define SOCK_STREAM 1
#define SOCK_DGRAM 2

// No send flags are used, should they be used
#define MSG_DONTWAIT_SEND 0x0040
#define MSG_DONTROUTE_SEND 0x0004

#define MSG_DONTWAIT_REV 0x40
#define MSG_PEEK_REV 0x2

#define INADDR_ANY 0x00000000

typedef int ssize_t;
typedef uint32_t socklen_t;
typedef uint16_t sa_family_t;

struct sockaddr {
    sa_family_t sa_family;
    char sa_data[14];
};

struct in_addr {
    uint32_t s_addr = 0;
};

struct sockaddr_in {
    sa_family_t sa_family = 0;
    uint16_t sin_port = 0;
    struct in_addr sin_addr;
    char pad[8];
};

struct ReceivedPacket {
    void* buffer;

    //Everything will be in host byte order
    size_t payload_len;
    uint32_t src_ip;
    uint16_t src_port;
    ReceivedPacket* next = nullptr;

    ReceivedPacket(void* buffer, size_t payload_len, uint32_t src_ip, uint16_t src_port)
        : buffer(buffer), payload_len(payload_len), src_ip(src_ip), src_port(src_port) {}

    ~ReceivedPacket() { next = nullptr; }
};

class UDPSocket {
   private:
    bool isbound = false;
    sockaddr_in sockaddr_info;
    StrongPtr<BB<ReceivedPacket*>> recieve_queue;

   public:
    UDPSocket();
    virtual ~UDPSocket();

    virtual void add(void* buffer, size_t payload_len, uint32_t src_ip, uint16_t src_port);
    virtual int bind(const sockaddr_in* addr);
    virtual ssize_t sendto(const void* buf, size_t len, const sockaddr_in* dst);
    virtual ssize_t recvfrom(void* buf, size_t len, sockaddr_in* src, int flags);
};

}  // namespace sockets