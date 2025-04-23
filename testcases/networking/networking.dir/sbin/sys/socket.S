# Networking Syscalls (Current range selected is 200-250, can easily be changed)

	# int socket(int domain, int type, int protocol);
	.global socket
socket:
	mov $200,%eax
	int $48
	ret

	# int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
	.global bind
bind:
	mov $201,%eax
	int $48
	ret
	# ssize_t sendto(int sockfd, const void* buf, size_t len,
             # int flags, const struct sockaddr *dest, socklen_t destlen);
	.global sendto
sendto:
	mov $202,%eax
	int $48
	ret

	# ssize_t recvfrom(int sockfd, void* buf, size_t len,
                # int flags, struct sockaddr *src, socklen_t *srclen);
	.global recvfrom
recvfrom:
	mov $203,%eax
	int $48
	ret

	# int setsockopt(int sockfd, int level, int optname,
               #   const void *optval, socklen_t optlen);
	.global setsockopt
setsockopt:
	mov $204,%eax
	int $48
	ret



	# extern int getsockname(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
	.global getsockname
getsockname:
	mov $204,%eax
	int $45
	ret



	# int close(int id)
	.global close
close:
	mov $6,%eax
	int $48
	ret
