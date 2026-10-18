B = build

# Prefer the bare-metal x86 toolchain (also works on Apple Silicon).
CROSS_COMPILE ?= $(shell command -v i686-elf-gcc >/dev/null 2>&1 && printf i686-elf-)
CC = $(CROSS_COMPILE)gcc
CXX = $(CROSS_COMPILE)g++
OBJCOPY = $(CROSS_COMPILE)objcopy

IMAGES = $B/kernel.img

all : ${IMAGES}

UTCS_OPT ?= -O3

ARCH_FLAGS = -m32 -ffreestanding -fno-pie -fno-stack-protector -mno-sse
CFLAGS = -std=c99 $(ARCH_FLAGS) -nostdlib -nostdinc -g ${UTCS_OPT} -Wall -Werror
CCFLAGS = -idirafter ../doom-sysroot/include -std=c++20 -fno-exceptions -fno-rtti $(ARCH_FLAGS) -nostdlib -g ${UTCS_OPT} -Wall -Werror

CFILES = $(wildcard *.c)
# Backup copies with spaces in their names are not translation units.
CCFILES := $(sort $(filter-out ./crti.cc ./crtn.cc, $(shell find . -name '*.cc' ! -name '* *' ! -path './build/*')))

SFILES = $(wildcard *.S) $(wildcard *.s)

ALL_OFILES = $(subst .cc,.o,$(CCFILES)) $(subst .c,.o,$(CFILES)) $(subst .s,.o,$(subst .S,.o,$(SFILES)))
KERNEL_OFILES = ${filter-out crtn.o, ${filter-out crti.o, ${ALL_OFILES}}}

OFILES = ${addprefix $B/,crti.o} ${shell $(CC) -m32 --print-file-name crtbegin.o} ${addprefix $B/,${KERNEL_OFILES}} ${shell $(CC) -m32 --print-file-name crtend.o}

# keep all files
.SECONDARY :

$B/%.o :  Makefile %.c
	@echo "compiling  $*.c"
	@mkdir -p build
	$(CC) -I. -c -MD -MF $B/$*.d -o $B/$*.o $(CFLAGS) $*.c

$B/%.o :  Makefile %.cc
	@echo "compiling  $*.cc"
	@mkdir -p $(dir $B/$*)
	$(CXX) -I. -c -MD -MF $B/$*.d -o $B/$*.o $(CCFLAGS) $*.cc

$B/%.o :  Makefile ${TESTS_DIR}/%.cc
	@echo "compiling  ${TESTS_DIR}/$*.cc"
	@mkdir -p build
	$(CXX) -I. -c -MD -MF $B/$*.d -o $B/$*.o $(CCFLAGS) ${TESTS_DIR}/$*.cc

$B/%.o :  Makefile %.S
	@echo "assembling $*.S"
	@mkdir -p build
	$(CC) -I. -MD -MF $B/$*.d -o $B/$*.o -m32 -c $*.S

$B/%.o :  Makefile %.s
	@echo "assembling $*.s"
	@mkdir -p build
	$(CC) -I. -MD -MF $B/$*.d -o $B/$*.o -m32 -c $*.s

$B/kernel.bin : Makefile $B/kernel
	@echo "producing  $B/$*.bin"
	@mkdir -p build
	$(OBJCOPY) -O binary $B/kernel $B/kernel.bin

$B/kernel.img : Makefile $B/kernel.bin
	@echo "producing  $B/kernel.img"
	@mkdir -p build
	dd if=$B/kernel.bin of=$B/kernel.img bs=512 conv=sync > /dev/null 2>&1
	@echo "---------------------------------"

clean ::
	rm -rf build

-include $(patsubst %.o,%.d,$(filter $B/%,$(OFILES)))

$B/kernel : script.ld Makefile $(OFILES)
	@echo "linking    $*.kernel"
	@mkdir -p build
	$(CXX) ${CCFLAGS} -nostdlib -static -Wl,--build-id=none -T script.ld -N -e start -o $@ $(OFILES) -lgcc

