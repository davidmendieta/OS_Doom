/* Single-process Newlib adapter. Kernel ABI is independent of libc headers. */
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <os/runtime.h>

extern int os_open(const char *, int, unsigned);
extern int os_close(int), os_length(int), os_mkdir(const char *);
extern int os_read(int, void *, unsigned), os_write(int, const void *, unsigned);
extern int os_file_seek(int, int, int), os_file_stat(int, os_file_info *);
extern void *os_allocate(unsigned);
extern void os_shutdown(void);

static int checked(int value, int error) {
    if (value < 0) { errno = error; return -1; }
    return value;
}

int open(const char *path, int flags, ...) {
    int mode = 0;
    if (flags & O_CREAT) {
        va_list args;
        va_start(args, flags);
        mode = va_arg(args, int);
        va_end(args);
    }
    /* The kernel uses BSD-style bits; don't pass libc's flags through. */
    int native = flags & O_ACCMODE;
    if (flags & O_CREAT) native |= 0x200;
    if (flags & O_TRUNC) native |= 0x400;
    if (flags & O_EXCL) { errno = ENOTSUP; return -1; }
    if (flags & O_APPEND) native |= 8;
    return checked(os_open(path, native, S_IFREG | mode), ENOENT);
}
int close(int fd) { return checked(os_close(fd), EBADF); }
_READ_WRITE_RETURN_TYPE read(int fd, void *buf, size_t size) { return checked(os_read(fd, buf, size), EIO); }
_READ_WRITE_RETURN_TYPE write(int fd, const void *buf, size_t size) { return checked(os_write(fd, buf, size), EIO); }
off_t lseek(int fd, off_t offset, int whence) {
    if (fd < 3) { errno = ESPIPE; return -1; }
    return checked(os_file_seek(fd, offset, whence), EINVAL);
}
int fstat(int fd, struct stat *st) {
    memset(st, 0, sizeof(*st));
    if (fd >= 0 && fd < 3) { st->st_mode = S_IFCHR | 0600; return 0; }
    os_file_info info;
    if (checked(os_file_stat(fd, &info), EBADF) < 0) return -1;
    st->st_mode = info.mode;
    st->st_size = info.size;
    st->st_ino = info.inode;
    st->st_nlink = info.links;
    st->st_blocks = info.blocks;
    st->st_blksize = info.block_size;
    return 0;
}
int stat(const char *path, struct stat *st) {
    int fd = open(path, O_RDONLY, 0);
    if (fd < 0) return -1;
    int result = fstat(fd, st);
    close(fd);
    return result;
}
int access(const char *path, int mode) {
    struct stat st;
    if (mode & ~(R_OK | W_OK | X_OK)) { errno = EINVAL; return -1; }
    if (stat(path, &st) < 0) return -1;
    if (((mode & R_OK) && !(st.st_mode & 0444)) ||
        ((mode & W_OK) && !(st.st_mode & 0222)) ||
        ((mode & X_OK) && !(st.st_mode & 0111))) { errno = EACCES; return -1; }
    return 0;
}
int isatty(int fd) { return fd >= 0 && fd < 3; }
int mkdir(const char *path, mode_t mode) { (void)mode; return checked(os_mkdir(path), EIO); }

void *sbrk(ptrdiff_t increment) {
    static char *base;
    static size_t used;
    const size_t capacity = 48 * 1024 * 1024;
    if (!base) {
        base = os_allocate(capacity);
        if (!base || base == (void *)-1) { base = NULL; errno = ENOMEM; return (void *)-1; }
    }
    if ((increment > 0 && (size_t)increment > capacity - used) ||
        (increment < 0 && (size_t)(-(int64_t)increment) > used)) {
        errno = ENOMEM;
        return (void *)-1;
    }
    char *result = base + used;
    used += increment;
    return result;
}
int getpid(void) { return 1; }
int kill(int pid, int signal) { (void)pid; (void)signal; errno = ENOSYS; return -1; }
int unlink(const char *path) { (void)path; errno = ENOSYS; return -1; }
int gettimeofday(struct timeval *tv, void *tz) {
    (void)tz;
    /* Uptime only; this OS does not yet provide a calendar clock. */
    uint32_t now = os_milliseconds();
    tv->tv_sec = now / 1000;
    tv->tv_usec = (now % 1000) * 1000;
    return 0;
}
void _exit(int status) {
    if (status) {
        static const char message[] = "*** userspace: FAIL abnormal exit\n";
        os_write(2, message, sizeof(message)-1);
    }
    os_shutdown();
    for (;;) {}
}
