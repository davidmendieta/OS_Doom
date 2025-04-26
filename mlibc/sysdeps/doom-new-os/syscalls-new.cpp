#include "syscalls-new.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <errno.h>
#include <poll.h>
#include "sys.h"

extern "C" int sys_access(const char *pathname, int mode) {
    return access(pathname, mode);
}

extern "C" int sys_arch_prctl(int code, unsigned long addr) {
    return arch_prctl(code, addr);
}

extern "C" int sys_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    return bind(sockfd, addr, addrlen);
}

extern "C" int sys_brk(void *addr) {
    return (int)brk(addr);
}

extern "C" int sys_clock_nanosleep(clock_t clock_id, int flags, const struct timespec *request, struct timespec *remaining) {
    return clock_nanosleep(clock_id, flags, request, remaining);
}

extern "C" pid_t sys_clone(int flags, void *child_stack, pid_t *parent_tid, pid_t *child_tid, unsigned long tls, int set_tid, int set_mask, int cmd) {
    return clone(flags, child_stack, parent_tid, child_tid, tls, set_tid, set_mask, cmd);
}

extern "C" int sys_close(int fd) {
    return close(fd);
}

extern "C" int sys_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    return connect(sockfd, addr, addrlen);
}

extern "C" int sys_execve(const char *filename, char *const argv[], char *const envp[]) {
    return execve(filename, argv, envp);
}

extern "C" void sys_exit(int status) {
    _exit(status);
}

extern "C" int sys_exit_group(int status) {
    return exit_group(status);
}

extern "C" int sys_fcntl(int fd, int cmd, unsigned long arg) {
    return fcntl(fd, cmd, arg);
}

extern "C" int sys_fstat(int fd, struct stat *statbuf) {
    return fstat(fd, statbuf);
}

extern "C" int sys_ftruncate(int fd, off_t length) {
    return ftruncate(fd, length);
}

extern "C" int sys_futex(int *uaddr, int op, int val, const struct timespec *timeout, int *uaddr2, int val3) {
    return futex(uaddr, op, val, timeout, uaddr2, val3);
}

extern "C" ssize_t sys_getdents64(unsigned int fd, struct linux_dirent64 *dirp, unsigned int count) {
    return getdents64(fd, dirp, count);
}

extern "C" int sys_getgid() {
    return getgid();
}

extern "C" int sys_getpeername(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    return getpeername(sockfd, addr, addrlen);
}

extern "C" int sys_getpid() {
    return getpid();
}

extern "C" int sys_getsockname(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    return getsockname(sockfd, addr, addrlen);
}

extern "C" int sys_getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen) {
    return getsockopt(sockfd, level, optname, optval, optlen);
}

extern "C" int sys_getuid() {
    return getuid();
}

extern "C" int sys_ioctl(int fd, unsigned long request, ...) {
    //how
}

extern "C" int sys_ioperm(unsigned long from, unsigned long num, int turn_on) {
    return ioperm(from, num, turn_on);
}

extern "C" off_t sys_lseek(int fd, off_t offset, int whence) {
    return lseek(fd, offset, whence);
}

extern "C" int sys_lstat(const char *path, struct stat *statbuf) {
    return lstat(path, statbuf);
}

extern "C" int sys_madvise(void *addr, size_t length, int advice) {
    return madvise(addr, length, advice);
}

extern "C" int sys_memfd_create(const char *name, unsigned int flags) {
    return memfd_create(name, flags);
}

extern "C" int sys_mkdir(const char *pathname, mode_t mode) {
    return mkdir(pathname, mode);
}

extern "C" void *sys_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset) {
    return mmap(addr, length, prot, flags, fd, offset);
}

extern "C" int sys_mprotect(void *addr, size_t len, int prot) {
    return mprotect(addr, len, prot);
}

extern "C" int sys_munmap(void *addr, size_t length) {
    return munmap(addr, length);
}

extern "C" int sys_openat(int dirfd, const char *pathname, int flags, mode_t mode) {
    return openat(dirfd, pathname, flags, mode);
}

extern "C" int sys_pipe2(int pipefd[2], int flags) {
    return pipe2(pipefd, flags);
}

extern "C" int sys_poll(struct pollfd *fds, nfds_t nfds, int timeout) {
    return poll(fds, nfds, timeout);
}

extern "C" int sys_ppoll(struct pollfd *fds, nfds_t nfds, const struct timespec *timeout, const sigset_t *sigmask) {
    return ppoll(fds, nfds, timeout, sigmask);
}

extern "C" int sys_prctl(int option, unsigned long arg2, unsigned long arg3, unsigned long arg4, unsigned long arg5) {
    return prctl(option, arg2, arg3, arg4, arg5);
}

extern "C" ssize_t sys_pread64(int fd, void *buf, size_t count, off_t offset) {
    return pread64(fd, buf, count, offset);
}

extern "C" int sys_prlimit64(pid_t pid, int resource, const struct rlimit64 *new_limit, struct rlimit64 *old_limit) {
    return prlimit64(pid, resource, new_limit, old_limit);
}

extern "C" ssize_t sys_read(int fd, void *buf, size_t count) {
    return read(fd, buf, count);
}

extern "C" ssize_t sys_readlink(const char *pathname, char *buf, size_t bufsize) {
    return readlink(pathname, buf, bufsize);
}

extern "C" ssize_t sys_recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen) {
    return recvfrom(sockfd, buf, len, flags, src_addr, addrlen);
}

extern "C" ssize_t sys_recvmsg(int sockfd, struct msghdr *msg, int flags) {
    return recvmsg(sockfd, msg, flags);
}

extern "C" int sys_rt_sigaction(int sig, const struct sigaction *act, struct sigaction *oldact, size_t sigsetsize) {
    return rt_sigaction(sig, act, oldact, sigsetsize);
}

extern "C" int sys_rt_sigprocmask(int how, const sigset_t *set, sigset_t *oldset, size_t sigsetsize) {
    return rt_sigprocmask(how, set, oldset, sigsetsize);
}

extern "C" int sys_select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout) {
    return select(nfds, readfds, writefds, exceptfds, timeout);
}

extern "C" ssize_t sys_sendmsg(int sockfd, const struct msghdr *msg, int flags) {
    return sendmsg(sockfd, msg, flags);
}

extern "C" ssize_t sys_sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen) {
    return sendto(sockfd, buf, len, flags, dest_addr, addrlen);
}

extern "C" int sys_set_robust_list(struct robust_list_head *head, size_t len) {
    return set_robust_list(head, len);
}

extern "C" int sys_setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen) {
    return setsockopt(sockfd, level, optname, optval, optlen);
}

extern "C" int sys_set_tid_address(int *tidptr) {
    return set_tid_address(tidptr);
}

extern "C" int sys_shmat(int shmid, const void *shmaddr, int shmflg) {
    return shmat(shmid, shmaddr, shmflg);
}

extern "C" int sys_shmctl(int shmid, int cmd, struct shmid_ds *buf) {
    return shmctl(shmid, cmd, buf);
}

extern "C" int sys_shmdt(const void *shmaddr) {
    return shmdt(shmaddr);
}

extern "C" int sys_shmget(key_t key, size_t size, int shmflg) {
    return shmget(key, size, shmflg);
}

extern "C" int sys_shutdown(int sockfd, int how) {
    return shutdown(sockfd, how);
}

extern "C" int sys_socket(int domain, int type, int protocol) {
    return socket(domain, type, protocol);
}

extern "C" int sys_stat(const char *path, struct stat *statbuf) {
    return stat(path, statbuf);
}

extern "C" int sys_umask(int mask) {
    return umask(mask);
}

extern "C" int sys_uname(struct utsname *buf) {
    return uname(buf);
}

extern "C" int sys_unlink(const char *pathname) {
    return unlink(pathname);
}

extern "C" ssize_t sys_write(int fd, const void *buf, size_t count) {
    return write(fd, buf, count);
}

extern "C" ssize_t sys_writev(int fd, const struct iovec *iov, int iovcnt) {
    return writev(fd, iov, iovcnt);
}

//nfds
//fix syscalls.h