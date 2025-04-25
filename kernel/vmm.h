#pragma once

#include "stdint.h"
#include "shared.h"
#include "physmem.h"
#include "vme.h"
// #include "blocking_lock.h"

class Node;
class BlockingLock;

namespace VMM {

    extern BlockingLock* shared_vme_lock;
    extern VME<BlockingLock>* shared_vme;

    extern void remove_PT_mapping(uint32_t* PD, uint32_t PDI);
    extern void remove_mapping(uint32_t* PD, uint32_t VPN);

    extern uint32_t new_page_directory();
    extern void copy_page_directory(uint32_t* pd, uint32_t* new_pd);

    // called (on the initial core) to initialize data structures, etc
    extern void global_init();

    // called on each core to do per-core initialization
    extern void per_core_init();

    extern void* naive_mmap(uint32_t size, bool shared, StrongPtr<Node> file, uint32_t file_offset, bool user);
    // extern void* naive_mmap_location(uint32_t size, bool shared, StrongPtr<Node> file, uint32_t file_offset, bool user, uint32_t location);

    void naive_munmap(void* p, bool user);
    void naive_munmap_location(void* p, bool user, uint32_t location);
    
    void* mmap(void* location, uint32_t size, uint32_t permissions, uint32_t flags, uint32_t fd, uint32_t offset)
    void munmap(void* p_, bool user, uint32_t size)
}

