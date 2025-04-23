// filepath: /u/kelly16/OS-Doom/kernel/uio.h
#ifndef UIO_H
#define UIO_H

#include "sys.h"
#include "stdint.h"

struct iovec {
    void  *iov_base;    /* Starting address */
    uint32_t iov_len;   /* Number of bytes to transfer */
};

#endif // UIO_H