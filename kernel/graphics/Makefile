CXX = g++
CXXFLAGS = -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector -nostartfiles -nodefaultlibs -Wall -Wextra -Werror -c -fno-exceptions -fno-rtti
INCLUDES = -I/usr/include -I/usr/local/include -I/usr/lib/gcc/x86_64-linux-gnu/9/include -I/usr/lib/gcc/x86_64-linux-gnu/9/include-fixed

all: vga_test.bin

vga_test.bin: vga.o vga_test.o
	$(CXX) -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector -nostartfiles -nodefaultlibs -T linker.ld -o vga_test.bin vga.o vga_test.o

vga.o: vga.cc vga.h
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o vga.o vga.cc

vga_test.o: vga_test.cc vga.h
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o vga_test.o vga_test.cc

clean:
	rm -f vga.o vga_test.o vga_test.bin

run: vga_test.bin
	qemu-system-i386 -kernel vga_test.bin -display curses -monitor stdio 