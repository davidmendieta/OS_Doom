//STILL NEED TO LIST INTERNAL-SYSDEPS.CPP in meson.build to support it - probably should wait for aarush to finish step 1 before doing this

// we need to implement (a subset of) functions whose headers are listed in 
// options/internal/include/mlibc/internal-sysdeps.hpp

#include <mlibc/internal-sysdeps.hpp>
#include <kernel/sys.h>
#include <kernel/heap.h>
#include <kernel/vmm.h>
#include <kernel/debug.h>

namespace mlibc {

    // 1) Logging & panic
    void sys_libc_log(const char *msg) {
        Debug::printf(msg);
    }

    [[noreturn]] void sys_libc_panic() {
        Debug::panic("mlibc panic\n");
    }
    
    // 2) TLS (if you don’t support threads yet, just return 0)
    int sys_tcb_set(void *pointer) {
        // e.g. current_thread->tls_base = pointer;
        return 0;
    }
    
    // 3) Futex stubs (if threads enabled, replace with real futex calls)
    // [[gnu::weak]] int sys_futex_tid()   { return -ENOSYS; }
    // int sys_futex_wait(int *uaddr, int val, const struct timespec *tmo) {
    //     (void)uaddr; (void)val; (void)tmo;
    //     return -ENOSYS;
    // }
    // int sys_futex_wake(int *uaddr) {
    //     (void)uaddr;
    //     return -ENOSYS;
    // }
    
    // 4) Anonymous memory (malloc / sbrk)
    int sys_anon_allocate(size_t size, void **ptr) {
        void *p = malloc(size);
        if (!p) return -1;
        *ptr = p;
        return 0;
    }
    int sys_anon_free(void *ptr, size_t size) {
        //free(ptr, size);
        free(ptr);
        return 0;
    }
    
    // 5) File I/O
    int sys_open(const char *path, int flags, mode_t mode, int *fd) {
        // int r = sys_openat(AT_FDCWD, path, flags, mode);
        int r = open(path, flags);
        if (r < 0) return r;
        *fd = r;
        return 0;
    }
    int sys_read(int fd, void *buf, size_t cnt, ssize_t *ret) {
        int r = read(fd, buf, cnt);
        if (r < 0) return r;
        *ret = r;
        return 0;
    }
    int sys_seek(int fd, off_t off, int whence, off_t *new_off) {
        off_t r = seek(fd, off);
        if (r < 0) return r;
        *new_off = r;
        return 0;
    }
    int sys_close(int fd) {
        return close(fd);
    }
    
    // Come back and fix it, currently not compatiable with naive_mmap
    // figure out what sys_stat do
    // // 6) stat (weak)
    // [[gnu::weak]] int sys_stat(fsfd_target t, int fd,
    //                            const char *path, int flags,
    //                            struct stat *st) {
    //     if (t == fsfd_target::path)
    //         return sys_stat(path, st);
    //     return -1;
    // }
    
    // // 7) Memory map (loader, SDL, etc.)
    // int sys_vm_map(void *hint, size_t sz, int prot, int flags,
    //                int fd, off_t off, void **res) {
    //     //void *m = VMM::map(hint, sz, prot, flags, fd, off);
    //     void *m = VMM::naive_mmap(sz, );
    //     if (!m) return -1;
    //     *res = m;
    //     return 0;
    // }

    int sys_vm_unmap(void *ptr, size_t sz) {
        VMM::naive_munmap(ptr, true); // put true for user (?)
        return 0;
    }
    [[gnu::weak]] int sys_vm_protect(void *ptr, size_t sz, int prot) {
        return -1;
    }
    
} // namespace mlibc

