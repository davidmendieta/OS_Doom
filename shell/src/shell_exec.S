    # shell_exec(char* path, uint32_t* argv, uint32_t argc)
    .global shell_exec
shell_exec:
    mov (%esp), %ebx
    mov 4(%esp), %ebx        # path
    mov 8(%esp), %edi        # argv
    mov 12(%esp), %esi       # argc

    dec %esi
    push $0                  # NULL terminator for argv[]

    .loop:
        mov (%edi,%esi,4), %eax
        push %eax
        dec %esi
        jns .loop

    push %ebx

    push $0

    mov $9,%eax
	int $48
	ret

    # TODO pop params on fail