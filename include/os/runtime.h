#ifndef OS_RUNTIME_H
#define OS_RUNTIME_H
#define OS_SYS_MILLISECONDS 110
#define OS_SYS_YIELD 111
#define OS_SYS_FILE_INFO 112
#define OS_SYS_FILE_SEEK 113
#define OS_SYS_ALLOCATE 114
#ifndef __ASSEMBLER__
#include <stdint.h>
typedef struct {
    uint32_t mode, size, inode, links, blocks, block_size;
} os_file_info;
uint32_t os_milliseconds(void);
void os_yield(void);
#endif
#endif
