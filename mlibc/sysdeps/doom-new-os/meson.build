#add_project_arguments('-D_GNU_SOURCE', language: 'cpp')

sysdep_supported_options = {
	'posix': true,
	'linux': false,
	'bsd': false,
	'glibc': false
}

arch_dir = host_machine.cpu_family()

doom_include_dirs = [
  include_directories('include'),
  include_directories('../../options/rtld/x86'),
  include_directories('include/mlibc/'),
  include_directories('../../options/elf/include'),
  include_directories('../../../kernel/'),
  include_directories('.'), # make sure the compiler can find syscalls-new.h and syscall_number.h
]

rtld_include_dirs += doom_include_dirs
libc_include_dirs += doom_include_dirs

# Include only low-level syscall handling
libc_sources += files(
	# Uncomment if you have this
	# arch_dir / 'syscall.cpp',
)

# You can add thread support later
# if get_option('posix_option').allowed()
# 	libc_sources += files(
# 		'generic/thread.cpp',
# 		arch_dir / 'cp_syscall.S',
# 		arch_dir / 'thread_entry.S'
# 	)
# endif

libc_sources += files(
	'internal-sysdeps.cpp',
	'syscalls-new.cpp' # only add cpp file to libc_source
)
#libc_sources += files('generic/sysdeps.cpp')


# Install ABI headers
if not no_headers
	install_headers(
		'include/abi-bits/auxv.h',
		'include/abi-bits/seek-whence.h',
		'include/abi-bits/time.h',
		'include/abi-bits/errno.h',
		'include/abi-bits/fcntl.h',
		'include/abi-bits/stat.h',
		'include/abi-bits/suseconds_t.h',
		'include/abi-bits/dev_t.h',
		'include/abi-bits/gid_t.h',
		'include/abi-bits/ino_t.h',
		'include/abi-bits/mode_t.h',
		'include/abi-bits/nlink_t.h',
		'include/abi-bits/pid_t.h',
		'include/abi-bits/uid_t.h',
		subdir: 'abi-bits',
		follow_symlinks: true
	)

	install_headers('include/syscall.h')
	install_headers('include/sys/syscall.h', subdir: 'sys')
	install_headers(
		'include/bits/syscall.h',
		'include/bits/syscall_aliases.h',
		subdir: 'bits'
	)
endif

# CRT objects
if not headers_only
	crt = custom_target('crt1',
		build_by_default: true,
		command: c_compiler.cmd_array() + ['-c', '-o', '@OUTPUT@', '@INPUT@'],
		input: arch_dir / 'crt-src/crt1.S',
		output: 'crt1.o',
		install: true,
		install_dir: get_option('libdir')
	)

	crt_pie = custom_target('Scrt1',
		build_by_default: true,
		command: c_compiler.cmd_array() + ['-fPIE', '-c', '-o', '@OUTPUT@', '@INPUT@'],
		input: arch_dir / 'crt-src/Scrt1.S',
		output: 'Scrt1.o',
		install: true,
		install_dir: get_option('libdir')
	)

	custom_target('crti',
		build_by_default: true,
		command: c_compiler.cmd_array() + ['-c', '-o', '@OUTPUT@', '@INPUT@'],
		input: arch_dir / 'crt-src/crti.S',
		output: 'crti.o',
		install: true,
		install_dir: get_option('libdir')
	)

	custom_target('crtn',
		build_by_default: true,
		command: c_compiler.cmd_array() + ['-c', '-o', '@OUTPUT@', '@INPUT@'],
		input: arch_dir / 'crt-src/crtn.S',
		output: 'crtn.o',
		install: true,
		install_dir: get_option('libdir')
	)

	wrapper_conf = configuration_data()
	wrapper_conf.set('LIBDIR', get_option('libdir'))
	wrapper_conf.set('PREFIX', get_option('prefix'))

	specs = configure_file(input: 'mlibc-gcc.specs.in',
		output: 'mlibc-gcc.specs',
		configuration: wrapper_conf)

	wrapper_script = configure_file(input: 'mlibc-gcc.in',
		output: 'mlibc-gcc',
		configuration: wrapper_conf)

	install_data(specs, install_dir: get_option('libdir'))
	install_data(wrapper_script, install_dir: get_option('bindir'))
endif
