
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




extern int socket(int domain, int type, int protocol);

extern int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

extern ssize_t sendto(int sockfd, const void* buf, size_t len,
               int flags, const struct sockaddr *dest, socklen_t destlen);

extern ssize_t recvfrom(int sockfd, void* buf, size_t len,
                 int flags, struct sockaddr *src, socklen_t *srclen);

extern int setsockopt(int sockfd, int level, int optname,
                  const void *optval, socklen_t optlen);


extern int getsockname(int sockfd, struct sockaddr* addr, socklen_t *addrlen);

extern int close(int fd);

