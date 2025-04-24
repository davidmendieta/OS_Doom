
#include "socket_sys.h"

#include "../../debug.h"
#include "../../ext2.h"
#include "../../threads.h"
#include "stdint.h"

namespace sockets {

bool address_valid(uint32_t addr, uint32_t size) {
    return addr >= 0x80000000 && addr + size <= 0xFFFFFFFF;
}

int socket_sys_handler(uint32_t eax, uint32_t* frame) {
    uint32_t* user_sp = (uint32_t*)frame[3];
    switch (eax) {
            // int socket(int domain, int type, int protocol);
        case 200: {
            int domain = user_sp[1];
            int type = user_sp[2];
            int protocol = user_sp[3];
            return socket(domain, type, protocol);
        }

        // extern int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
        case 201: {
            int sockfd = user_sp[1];
            if (sockfd < 0) {
                return -1;
            }
            const struct sockaddr* addr = (const struct sockaddr*)user_sp[2];
            socklen_t addrlen = user_sp[3];
            return bind((uint32_t)sockfd, addr, addrlen);
        }

        // extern ssize_t sendto(int sockfd, const void* buf, size_t len,
        // int flags, const struct sockaddr *dest, socklen_t destlen);
        case 202: {
            int sockfd = user_sp[1];
            const void* buf = (const void*)user_sp[2];
            uint32_t len = (uint32_t)user_sp[3];
            int flags = user_sp[4];
            const struct sockaddr* dest = (const struct sockaddr*)user_sp[5];
            socklen_t destlen = user_sp[6];
            return sendto((uint32_t)sockfd, buf, len, flags, dest, destlen);
        }

        // extern ssize_t recvfrom(int sockfd, void* buf, size_t len,
        // int flags, struct sockaddr *src, socklen_t *srclen);
        case 203: {
            int sockfd = user_sp[1];
            void* buf = (void*)user_sp[2];
            uint32_t len = (uint32_t)user_sp[3];
            int flags = user_sp[4];
            struct sockaddr* src = (struct sockaddr*)user_sp[5];
            socklen_t* srclen = (socklen_t*)user_sp[6];
            return recvfrom((uint32_t)sockfd, buf, len, flags, src, srclen);
        }

        // extern int setsockopt(int sockfd, int level, int optname,
        // const void *optval, socklen_t optlen);
        case 204: {
            int sockfd = user_sp[1];
            if (sockfd < 0) {
                return -1;
            }
            const struct sockaddr* addr = (const struct sockaddr*)user_sp[2];
            socklen_t addrlen = user_sp[3];
            return bind((uint32_t)sockfd, addr, addrlen);
        }

        // extern int getsockname(uint32_t sockfd, struct sockaddr *sraddr, socklen_t *addrlen);
        case 205: {
            int sockfd = user_sp[1];
            if (sockfd < 0) {
                return -1;
            }
            struct sockaddr* addr = (struct sockaddr*)user_sp[2];
            socklen_t addrlen = user_sp[3];
            return getsockname((uint32_t)sockfd, addr, &addrlen);
        }

        default: {
            Debug::printf("*** unknown system call inside Socket %d\n", eax);
            return -1;
        }
    }
}

int socket(int domain, int type, int protocol) {
    if (domain != AF_INET || type != SOCK_DGRAM) {
        return -1;
    }
    auto me = impl::threads::state.current();
    StrongPtr<UDPSocket> new_udp_socket = StrongPtr<UDPSocket>::make();
    for (uint32_t i = 0; i < PCB_ARR_SIZE; i++) {
        if (me->pcb->sockdt[i] == nullptr) {  // Found empty file
            me->pcb->sockdt[i] = new_udp_socket;
            return (i + (3 * PCB_ARR_SIZE));
        }
    }
    Debug::printf("Sockets are FULL!\n");
    return -1;
}

int bind(uint32_t sockfd, const struct sockaddr* addr, socklen_t addrlen) {
    if (!address_valid((uint32_t)addr, addrlen)) {
        return -1;
    }
    auto me = impl::threads::state.current();
    if (sockfd < (3 * PCB_ARR_SIZE) || sockfd >= (4 * PCB_ARR_SIZE)) {
        return -1;
    }
    sockfd -= (3 * PCB_ARR_SIZE);
    if (me->pcb->sockdt[sockfd] == nullptr) {  // Found empty file
        return -1;
    }

    StrongPtr<UDPSocket> socket = me->pcb->sockdt[sockfd];
    return socket->bind((const struct sockaddr_in*)addr);
}

ssize_t sendto(uint32_t sockfd, const void* buf, size_t len, int flags, const struct sockaddr* dest,
               socklen_t destlen) {
    if (!address_valid((uint32_t)buf, len) || !address_valid((uint32_t)dest, destlen)) {
        return -1;
    }
    auto me = impl::threads::state.current();

    if (sockfd < (3 * PCB_ARR_SIZE) || sockfd >= (4 * PCB_ARR_SIZE)) {
        return -1;
    }
    sockfd -= (3 * PCB_ARR_SIZE);
    if (me->pcb->sockdt[sockfd] == nullptr) {  // Found empty file
        return -1;
    }
    StrongPtr<UDPSocket> socket = me->pcb->sockdt[sockfd];
    return socket->sendto(buf, len, (const struct sockaddr_in*)dest);
}

ssize_t recvfrom(uint32_t sockfd, void* buf, size_t len, int flags, struct sockaddr* src,
                 socklen_t* srclen) {
    if (!address_valid((uint32_t)buf, len) || !address_valid((uint32_t)src, sizeof(sockaddr_in)) ||
        !address_valid((uint32_t)srclen, sizeof(uint32_t))) {
        return -1;
    }
    auto me = impl::threads::state.current();
    if (sockfd < (3 * PCB_ARR_SIZE) || sockfd >= (4 * PCB_ARR_SIZE)) {
        return -1;
    }
    sockfd -= (3 * PCB_ARR_SIZE);
    if (me->pcb->sockdt[sockfd] == nullptr) {  // Found empty file
        return -1;
    }
    StrongPtr<UDPSocket> socket = me->pcb->sockdt[sockfd];
    return socket->recvfrom(buf, len, (struct sockaddr_in*)src, flags);
}

int getsockname(uint32_t sockfd, struct sockaddr* addr, socklen_t* addrlen) {
    if (!address_valid((uint32_t)addrlen, sizeof(uint32_t)) ||
        !address_valid((uint32_t)addr, *addrlen)) {
        return -1;
    }
    auto me = impl::threads::state.current();
    if (sockfd < (3 * PCB_ARR_SIZE) || sockfd >= (4 * PCB_ARR_SIZE)) {
        return -1;
    }
    sockfd -= (3 * PCB_ARR_SIZE);
    if (me->pcb->sockdt[sockfd] == nullptr) {
        return -1;
    }
    StrongPtr<UDPSocket> socket = me->pcb->sockdt[sockfd];
    return socket->getsockname((struct sockaddr_in*)addr);
}

}  // namespace sockets
