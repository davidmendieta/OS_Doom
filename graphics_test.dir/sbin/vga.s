# io.s -- assembly implementations of inb and outb for VGA I/O

    .global inb
    .global outb

# uint8_t inb(uint16_t port)
inb:
    movl 4(%esp), %edx       # get port argument into edx
    xorl %eax, %eax          # zero out eax
    inb %dx, %al             # read from port into al
    ret

# void outb(uint16_t port, uint8_t val)
outb:
    movl 4(%esp), %edx       # port -> edx
    movl 8(%esp), %eax       # value -> eax
    outb %al, %dx            # write al to port dx
    ret