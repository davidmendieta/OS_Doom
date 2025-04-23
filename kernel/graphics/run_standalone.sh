#!/bin/bash

# Set the compiler path
PATH=~gheith/public/cs439/bin:$PATH
LD_LIBRARY_PATH=~gheith/public/cs439/lib64:$LD_LIBRARY_PATH

# Compile the standalone test
make -f Makefile.standalone clean
make -f Makefile.standalone

# Run the test in QEMU
qemu-system-i386 -kernel vga_test_standalone.bin -display curses -monitor stdio 