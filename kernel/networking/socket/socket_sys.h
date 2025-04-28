
#include <stdint.h>

#include "socket.h"

namespace sockets {

extern int socket_sys_handler(uint32_t eax, uint32_t* frame);

extern int socket(int domain, int type, int protocol);

extern int bind(uint32_t sockfd, const struct sockaddr* addr, socklen_t addrlen);

extern ssize_t sendto(uint32_t sockfd, const void* buf, size_t len, int flags,
                      const struct sockaddr* dest, socklen_t destlen);

extern ssize_t recvfrom(uint32_t sockfd, void* buf, size_t len,
        int flags, struct sockaddr *src, socklen_t *srclen);
        

extern int getsockname(uint32_t sockfd, struct sockaddr *addr, socklen_t *addrlen);

extern int getpeername(uint32_t sockfd, struct sockaddr *addr, socklen_t *addrlen);

extern int connect(uint32_t sockfd, const struct sockaddr* address, socklen_t address_len);



}  // namespace sockets