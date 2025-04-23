#include "sys.h"
#include "stdint.h"

// new code
// struct for iovec
struct iovec {
    void  *iov_base;    /* Starting address */
    size_t iov_len;     /* Number of bytes to transfer */
};