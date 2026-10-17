
#include "stdint.h"
/************************/
/* Socket System calls */
/***********************/

typedef int ssize_t;
typedef unsigned int size_t;
typedef uint32_t socklen_t;


#define AF_UNSPEC 0
#define AF_INET 2

#define SOCK_STREAM 1
#define SOCK_DGRAM 2

#define INADDR_ANY ((uint32_t)0x00000000)


typedef int ssize_t;
typedef uint32_t socklen_t;
typedef uint16_t sa_family_t;

struct sockaddr {
    sa_family_t sin_family;
    char sa_data[14];
};

struct in_addr {
    uint32_t s_addr;
};

struct sockaddr_in {
    sa_family_t sin_family;
    uint16_t sin_port;
    struct in_addr sin_addr;
    char pad[8];
};


static inline uint16_t ntohs(uint16_t netshort) {
    return (netshort << 8) | (netshort >> 8);
}

static inline uint16_t htons(uint16_t hostshort) {
    return (hostshort << 8) | (hostshort >> 8);
}

static inline uint32_t ntohl(uint32_t netlong) {
    return ((netlong & 0x000000FF) << 24) | ((netlong & 0x0000FF00) << 8) |
           ((netlong & 0x00FF0000) >> 8) | ((netlong & 0xFF000000) >> 24);
}

static inline uint32_t htonl(uint32_t hostlong) {
    return ((hostlong & 0x000000FF) << 24) | ((hostlong & 0x0000FF00) << 8) |
           ((hostlong & 0x00FF0000) >> 8) | ((hostlong & 0xFF000000) >> 24);
}


extern int socket(int domain, int type, int protocol);

extern int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

extern ssize_t sendto(int sockfd, const void* buf, size_t len,
               int flags, const struct sockaddr *dest, socklen_t destlen);

extern ssize_t recvfrom(int sockfd, void* buf, size_t len,
                 int flags, struct sockaddr *src, socklen_t *srclen);

extern int setsockopt(int sockfd, int level, int optname,
                  const void *optval, socklen_t optlen);


extern int getsockname(int sockfd, struct sockaddr* addr, socklen_t *addrlen);


