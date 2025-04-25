// #ifndef MLIBC_THREAD_ENTRY

// #include <mlibc/tcb.hpp>

// extern "C" int __mlibc_spawn_thread(int flags, void *stack, void *pid_out, void *child_tid, void *tcb);
// extern "C" void __mlibc_enter_thread(void *entry, void *user_arg);

// namespace mlibc {
// 	void *prepare_stack(void *entry, void *user_arg);
// }

// #endif // MLIBC_THREAD_ENTRY
#pragma once
typedef unsigned long ioctl_request_t;
static inline int ioctl(int fd, ioctl_request_t req, void *arg) {
  // leave unimplemented or hook through your kernel’s syscall
  return -1;
}