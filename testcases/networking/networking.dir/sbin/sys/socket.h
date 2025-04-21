
#include "stdint.h"

/************************/
/* Socket System calls */
/***********************/

typedef int ssize_t;
typedef unsigned int size_t;
typedef uint32_t socklen_t;


extern int socket(int domain, int type, int protocol);

extern int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

extern ssize_t sendto(int sockfd, const void* buf, size_t len,
               int flags, const struct sockaddr *dest, socklen_t destlen);

extern ssize_t recvfrom(int sockfd, void* buf, size_t len,
                 int flags, struct sockaddr *src, socklen_t *srclen);

extern int setsockopt(int sockfd, int level, int optname,
                  const void *optval, socklen_t optlen);

extern int close(int fd);

