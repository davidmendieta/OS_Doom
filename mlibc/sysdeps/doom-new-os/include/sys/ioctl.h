#pragma once
#include <abi-bits/ioctls.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

int ioctl(int fd, unsigned long request, ...);

#ifdef __cplusplus
}
#endif
