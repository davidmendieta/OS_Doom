#include <os/runtime.h>
#include <os/input.h>
    .text
    .macro syscall name, number
    .global \name
\name:
    mov $\number, %eax
    int $48
    ret
    .endm
    syscall os_write, 1
    syscall os_close, 6
    syscall os_shutdown, 7
    syscall os_open, 10
    syscall os_length, 11
    syscall os_read, 12
    syscall os_seek, 13
    syscall os_mkdir, 15
    syscall os_allocate, OS_SYS_ALLOCATE
    syscall os_milliseconds, OS_SYS_MILLISECONDS
    syscall os_yield, OS_SYS_YIELD
    syscall os_input_read, OS_SYS_INPUT_READ
    syscall os_file_stat, OS_SYS_FILE_INFO
    syscall os_file_seek, OS_SYS_FILE_SEEK
    .section .note.GNU-stack,"",@progbits
