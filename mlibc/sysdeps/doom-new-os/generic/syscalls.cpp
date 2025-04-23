#include <mlibc/internal-sysdeps.hpp>
#include <stddef.h>
#include <stdint.h>

namespace mlibc {

void sys_libc_log(const char *msg) {
	// Just drop it for now or send to some debug port
}

void sys_libc_panic() {
	while (1); // hang
}

int sys_anon_allocate(size_t size, void **pointer) {
	// TODO: hook into your kernel’s allocator
	*pointer = (void *)0x400000; // fake address
	return 0;
}

int sys_anon_free(void *, size_t) {
	return 0;
}

int sys_tcb_set(void *) {
	return 0;
}

int sys_futex_wake(int *) {
	return 0;
}

int sys_futex_wait(int *, int, const struct timespec *) {
	return 0;
}

int sys_open(const char *, int, mode_t, int *fd) {
	*fd = -1;
	return 0;
}

int sys_close(int) {
	return 0;
}

int sys_seek(int, off_t, int, off_t *new_offset) {
	*new_offset = 0;
	return 0;
}

int sys_read(int, void *, size_t, int *bytes_read) {
	*bytes_read = 0;
	return 0;
}

int sys_vm_map(void *, size_t, int, int, int, off_t, void **window) {
	*window = (void *)0x500000;
	return 0;
}

}
