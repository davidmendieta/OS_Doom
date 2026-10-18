    .text
    .global start
start:
    # The kernel supplies argc and argv on the initial user stack.
    call main
    mov $7, %eax
    int $48
1:  jmp 1b

    .macro syscall name, number
    .global \name
\name:
    mov $\number, %eax
    int $48
    ret
    .endm

    syscall write, 1
    syscall close, 6
    syscall open, 10
    syscall read, 12

    .section .note.GNU-stack,"",@progbits
