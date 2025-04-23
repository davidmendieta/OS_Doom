#!/bin/bash

# Set the compiler path
PATH=~gheith/public/cs439/bin:$PATH
LD_LIBRARY_PATH=~gheith/public/cs439/lib64:$LD_LIBRARY_PATH

# Build the kernel
cd kernel
make clean
make

# Run the kernel in QEMU
qemu-system-i386 -kernel build/kernel.img -display curses -monitor stdio 