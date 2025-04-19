project('os_port', 'c', 'cpp', version: '1.0.0')

cc = meson.get_compiler('c')
cpp = meson.get_compiler('cpp')

CFLAGS = ['-std=c99', '-m32', '-nostdlib', '-nostdinc', '-g', '-O3', '-Wall', '-Werror', '-mno-sse']
CCFLAGS = ['-std=c++20', '-fno-exceptions', '-fno-rtti', '-m32', '-ffreestanding', '-nostdlib', '-g', '-O3', '-Wall', '-Werror', '-mno-sse']

CFILES = files('*.c')
CCFILES = files('*.cc')
SFILES = files('*.S', '*.s')

ALL_OFILES = []
foreach src : CFILES
  ALL_OFILES += [src.splitext()[0] + '.o']
endforeach

foreach src : CCFILES
  ALL_OFILES += [src.splitext()[0] + '.o']
endforeach

foreach src : SFILES
  ALL_OFILES += [src.splitext()[0] + '.o']
endforeach

KERNEL_OFILES = ALL_OFILES - files('crtn.o', 'crti.o')

B = 'build'
build_dir = meson.build_root() / B 

# Compile the C, C++, and Assembly files (with custom targets)
foreach src : CFILES
  object_file = build_dir / src.basename() + '.o'
  custom_target(src.basename() + '_obj', 
    input: src,
    output: object_file,
    command: [cc, '-I.', '-c', '-MD', '-MF', object_file + '.d', '-o', object_file] + CFLAGS
  )
endforeach

foreach src : CCFILES
  object_file = build_dir / src.basename() + '.o'
  custom_target(src.basename() + '_obj', 
    input: src,
    output: object_file,
    command: [cpp, '-I.', '-c', '-MD', '-MF', object_file + '.d', '-o', object_file] + CCFLAGS
  )
endforeach

foreach src : SFILES
  object_file = build_dir / src.basename() + '.o'
  custom_target(src.basename() + '_obj', 
    input: src,
    output: object_file,
    command: [cc, '-I.', '-c', '-MD', '-MF', object_file + '.d', '-o', object_file, '-m32'] 
  )
endforeach

# Link the kernel object files and create the kernel image
kernel_output = build_dir / 'kernel'
executable('kernel', KERNEL_OFILES, 
  link_with: [cc],
  cflags: CFLAGS + ['-nostdlib', '-T', 'script.ld', '-e', 'start'],
  install: false
)

# Convert kernel to binary
custom_target('kernel_bin', 
  input: kernel_output,
  output: build_dir / 'kernel.bin',
  command: ['objcopy', '-O', 'binary', kernel_output, build_dir / 'kernel.bin']
)

# Create kernel image
custom_target('kernel_img', 
  input: build_dir / 'kernel.bin',
  output: build_dir / 'kernel.img',
  command: ['dd', 'if=' + build_dir / 'kernel.bin', 'of=' + build_dir / 'kernel.img', 'bs=512', 'conv=sync']
)

# Clean the build directory
clean_target = custom_target('clean', 
  command: ['rm', '-rf', build_dir]
)

