#ifndef _SYS_CALLS_H_
#define _SYS_CALLS_H_

extern "C" int sys_access(const char *pathname, int mode);
extern "C" int sys_arch_prctl(int code, unsigned long addr);
extern "C" int sys_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
extern "C" int sys_brk(void *end_data_segment);
extern "C" int sys_clock_nanosleep(clock_t clock_id, int flags, const struct timespec *request, struct timespec *remaining);
extern "C" pid_t sys_clone(int flags, void *child_stack, pid_t *parent_tid, pid_t *child_tid, unsigned long tls, int set_tid, int set_mask, int cmd);
extern "C" int sys_close(int fd);
extern "C" int sys_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
extern "C" int sys_execve(const char *filename, char *const argv[], char *const envp[]);
extern "C" void sys_exit(int status);
extern "C" int sys_exit_group(int status);
extern "C" int sys_fcntl(int fd, int cmd, unsigned long arg);
extern "C" int sys_fstat(int fd, struct stat *statbuf);
extern "C" int sys_ftruncate(int fd, off_t length);
extern "C" int sys_futex(int *uaddr, int op, int val, const struct timespec *timeout, int *uaddr2, int val3);
extern "C" ssize_t sys_getdents64(uint32_t fd, struct linux_dirent64 *dirp, unsigned int count);
extern "C" int sys_getgid();
extern "C" int sys_getpeername(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
extern "C" int sys_getpid();
extern "C" int sys_getsockname(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
extern "C" int sys_getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen);
extern "C" int sys_getuid();
extern "C" int sys_ioctl(int fd, unsigned long request, ...);
extern "C" int sys_ioperm(unsigned long from, unsigned long num, int turn_on);
extern "C" off_t sys_lseek(int fd, off_t offset, int whence);
extern "C" int sys_lstat(const char *path, struct stat *statbuf);
extern "C" int sys_madvise(void *addr, size_t length, int advice);
extern "C" int sys_memfd_create(const char *name, unsigned int flags);
extern "C" int sys_mkdir(const char *pathname, mode_t mode);
extern "C" void *sys_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
extern "C" int sys_mprotect(void *addr, size_t len, int prot);
extern "C" int sys_munmap(void *addr, size_t length);
extern "C" int sys_openat(int dirfd, const char *pathname, int flags, mode_t mode);
extern "C" int sys_pipe2(int pipefd[2], int flags);
extern "C" int sys_poll(struct pollfd *fds, nfds_t nfds, int timeout);
extern "C" int sys_ppoll(struct pollfd *fds, nfds_t nfds, const struct timespec *timeout, const sigset_t *sigmask);
extern "C" int sys_prctl(int option, unsigned long arg2, unsigned long arg3, unsigned long arg4, unsigned long arg5);
extern "C" ssize_t sys_pread64(int fd, void *buf, size_t count, off_t offset);
extern "C" int sys_prlimit64(pid_t pid, int resource, const struct rlimit64 *new_limit, struct rlimit64 *old_limit);
extern "C" ssize_t sys_read(int fd, void *buf, size_t count);
extern "C" ssize_t sys_readlink(const char *pathname, char *buf, size_t bufsize);
extern "C" ssize_t sys_recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
extern "C" ssize_t sys_recvmsg(int sockfd, msghdr *msg, int flags); // not sure
extern "C" int sys_rt_sigaction(int sig, const struct sigaction *act, struct sigaction *oldact, size_t sigsetsize);
extern "C" int sys_rt_sigprocmask(int how, const sigset_t *set, sigset_t *oldset, size_t sigsetsize);
extern "C" int sys_select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
extern "C" ssize_t sys_sendmsg(int sockfd, const msghdr *msg, int flags); // not sure
extern "C" ssize_t sys_sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
extern "C" int sys_set_robust_list(struct robust_list_head *head, size_t len);
extern "C" int sys_setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen);
extern "C" int sys_set_tid_address(int *tidptr);
extern "C" int sys_shmat(int shmid, const void *shmaddr, int shmflg);
extern "C" int sys_shmctl(int shmid, int cmd, struct shmid_ds *buf);
extern "C" int sys_shmdt(const void *shmaddr);
extern "C" int sys_shmget(key_t key, size_t size, int shmflg);
extern "C" int sys_shutdown(int sockfd, int how);
extern "C" int sys_socket(int domain, int type, int protocol);
extern "C" int sys_stat(const char *path, struct stat *statbuf);
extern "C" int sys_umask(int mask);
extern "C" int sys_uname(struct utsname *buf);
extern "C" int sys_unlink(const char *pathname);
extern "C" ssize_t sys_write(int fd, const void *buf, size_t count);
extern "C" ssize_t sys_writev(int fd, const iovec *iov, int iovcnt); // not sure

#endif // _SYS_CALLS_H_