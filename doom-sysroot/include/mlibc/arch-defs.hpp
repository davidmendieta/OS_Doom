#pragma once
#include <cstdint>

//#define __MLIBC_ABI_ONLY
#define __MLIBC_POSIX_OPTION

#pragma once

// Assume 32-bit for Doom
typedef uint32_t elf_addr;
typedef uint32_t elf_off;
typedef uint16_t elf_half;
typedef uint32_t elf_word;
typedef int32_t  elf_sword;

typedef struct {
    elf_sword d_tag;
    union {
        elf_word d_val;
        elf_addr d_ptr;
    } d_un;
} elf_dyn;

typedef struct {
    elf_word st_name;
    elf_addr st_value;
    elf_word st_size;
    unsigned char st_info;
    unsigned char st_other;
    elf_half st_shndx;
} elf_sym;
