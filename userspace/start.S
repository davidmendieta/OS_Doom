    .text
    .global start
start:
    # Initial stack holds argc, argv. The C runtime supplies environment/stdio.
    call main
    push %eax
    call exit
1:  jmp 1b
    .section .note.GNU-stack,"",@progbits
