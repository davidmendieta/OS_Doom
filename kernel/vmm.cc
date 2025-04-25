#include "vmm.h"
#include "machine.h"
#include "idt.h"
#include "libk.h"
#include "blocking_lock.h"
#include "config.h"
#include "threads.h"
#include "debug.h"
#include "ext2.h"
#include "sys.h"
#include "page_cache.h"

namespace VMM {

    #define NONE  0x0       
    #define READ  0x1       
    #define WRITE 0x2       
    #define EXEC  0x4       

    #define SHARED     0x01  
    #define PRIVATE    0x02  
    #define FIXED      0x10  
    #define ANON       0x20  

    BlockingLock* shared_vme_lock = nullptr;
    VME<BlockingLock>* shared_vme = nullptr;

    bool check_VPN(uint32_t* PD, uint32_t VPN) {
        uint32_t PDI = VPN >> 10;
        uint32_t PTI = VPN & (0x3FF);

        // Debug::printf("CONVERTING VA %x --> PDI %x + PTI %x\n", VPN << 12, PDI, PTI);

        uint32_t PDE = PD[PDI];

        uint32_t* PT;
        // Present
        if(!(PDE & 0x1)) return false;

        PT = (uint32_t*)((PDE >> 12) << 12);

        return PT[PTI] & 0x1;
    }

    void add_PT_mapping(uint32_t* PD, uint32_t PDI, bool global, bool write, bool user_mode) {
        uint32_t PDE = PD[PDI];
        if(!(PDE & 0x1)) {
            uint32_t PT = PhysMem::alloc_frame();
            PD[PDI] |= 0x1;
            if(global) PD[PDI] |= 0x100; // global
            if(write) PD[PDI] |= 0x2;
            if(user_mode) PD[PDI] |= 0x4;
            PD[PDI] &= 0xFFF;
            PD[PDI] |= PT;
        }
    }

    void remove_PT_mapping(uint32_t* PD, uint32_t PDI) {
        uint32_t PDE = PD[PDI];
        if(PDE & 0x1) {
            PhysMem::dealloc_frame(PD[PDI] & ~0xFFF);
            PD[PDI] &= 0;
        }
    }

    void add_mapping(uint32_t* PD, uint32_t VPN, uint32_t PPN, bool global, bool write, bool user_mode) {
        uint32_t PDI = VPN >> 10;
        uint32_t PTI = VPN & (0x3FF);

        // Debug::printf("CONVERTING VA %x --> PDI %x + PTI %x\n", VPN << 12, PDI, PTI);

        uint32_t PDE = PD[PDI];

        uint32_t* PT;
        // Present
        if(PDE & 0x1) {
            PT = (uint32_t*)((PDE >> 12) << 12);
        } else {
            PT = (uint32_t*)PhysMem::alloc_frame();
            // Debug::printf("PT: %x\n", PT);
            // ASSERT((uint32_t)PT == PhysMem::framedown((uint32_t)PT));
            PD[PDI] |= 0x1;
            if(global) PD[PDI] |= 0x100; // global
            if(write) PD[PDI] |= 0x2;
            if(user_mode) PD[PDI] |= 0x4;
            PD[PDI] &= 0xFFF;
            PD[PDI] |= ((uint32_t)PT);
        }

        PT[PTI] &= 0x0;
        PT[PTI] |= 0x1; // valid
        PT[PTI] |= 0x20;  // access bit
        if(global) PT[PTI] |= 0x100; // global
        if(write) PT[PTI] |= 0x2;
        if(user_mode) PT[PTI] |= 0x4;
        PT[PTI] |= (PPN << 12);

        invlpg(VPN << 12);
    }

    void remove_mapping(uint32_t* PD, uint32_t VPN) {
        ASSERT(PD != nullptr);
        uint32_t PDI = VPN >> 10;
        uint32_t PTI = VPN & (0x3FF);
        
        uint32_t PDE = PD[PDI];
        
        uint32_t* PT;
        // Present
        if(PDE & 0x1) {
            PT = (uint32_t*)((PDE >> 12) << 12);
        } else {
            return;
        }
        
        if(PT[PTI] & 0x1) {
            PhysMem::dealloc_frame(PT[PTI] & ~0xFFF);
        }

        PT[PTI] = 0x0;
        invlpg(VPN << 12);
        return;
    }

    uint32_t global_page_directory;

    uint32_t new_page_directory() {
        // Debug::printf("CREATING A NEW PD\n");
        uint32_t tmp = PhysMem::alloc_frame();
        // Debug::printf("CREATED PD %x\n", tmp);
        memcpy((void*)tmp, (void*)global_page_directory, PhysMem::FRAME_SIZE);
        return (uint32_t)tmp;
    }

    void copy_page_directory(uint32_t* pd, uint32_t* new_pd) {
        for(uint32_t i = (0x80000000 >> 22); i < (0xF0000000 >> 22); i++) {
            if(pd[i] & 0x1) { // PT ENTRY IS VALID
                uint32_t* pt = (uint32_t*)(pd[i] & ~0xFFF);
                uint32_t* new_pt = (uint32_t*)PhysMem::alloc_frame();
                new_pd[i] = pd[i] & 0xFFF; // copy metadata
                new_pd[i] |= (uint32_t)new_pt; // assign new PT
                for(uint32_t y = 0; y < 1024; y++) {
                    if(pt[y] & 0x1) {
                        uint32_t* pa = (uint32_t*)(pt[y] & ~0xFFF);
                        uint32_t* new_pa = (uint32_t*)PhysMem::alloc_frame();
                        new_pt[y] = pt[y] & 0xFFF; // copy metadata
                        new_pt[y] |= (uint32_t)new_pa; // assign new PT

                        memcpy(new_pa, pa, PhysMem::FRAME_SIZE);
                    }
                }
                
            }
        }
    }

    void global_init() {
        global_page_directory = PhysMem::alloc_frame();
        // Debug::printf("GLOBAL PAGE %x\n", global_page_directory);

        uint32_t num_identity_maps =  kConfig.memSize >> 12;
        // Debug::printf("NUM KERNEL PAGES: %d\n", num_kernel_pages);
        // uint32_t num_shared_pages = PhysMem::ppn(0xFFFFFFFF - 0xF0000000 + 0x1);

        // Kernel Pages
        for(uint32_t i = 1; i < num_identity_maps; i++) {
            add_mapping((uint32_t*)global_page_directory, i, i, true, false, false);
        }

        for(uint32_t i = (0xF0000000 >> 22); i <= (0xFFFFFFFF >> 22); i++) {
            add_PT_mapping((uint32_t*)global_page_directory, i, true, true, true);
        }

        add_mapping((uint32_t*)global_page_directory, PhysMem::ppn(kConfig.ioAPIC), PhysMem::ppn(kConfig.ioAPIC), true, false, true);
        add_mapping((uint32_t*)global_page_directory, PhysMem::ppn(kConfig.localAPIC), PhysMem::ppn(kConfig.localAPIC), true, false, true);
        
        shared_vme_lock = new BlockingLock();
        shared_vme = new VME<BlockingLock>(kConfig.localAPIC + PhysMem::FRAME_SIZE, 0xFFFFFFFF);

        shared_vme->insert_free_space(0xF0000000, (kConfig.ioAPIC - 0xF0000000) / PhysMem::FRAME_SIZE);
        shared_vme->insert_entry_sorted(kConfig.ioAPIC, 1, PhysMem::FRAME_SIZE, StrongPtr<Node>{}, 0, false);
        shared_vme->insert_free_space(kConfig.ioAPIC + PhysMem::FRAME_SIZE, (kConfig.localAPIC - (kConfig.ioAPIC + PhysMem::FRAME_SIZE)) / PhysMem::FRAME_SIZE);
        shared_vme->insert_entry_sorted(kConfig.localAPIC, 1, PhysMem::FRAME_SIZE, StrongPtr<Node>{}, 0, false);
        // // Shared
        // for(uint32_t i = 0; i < num_shared_pages; i++) {
        //     add_mapping((uint32_t*)global_page_directory, i + PhysMem::ppn(0xF0000000), i + PhysMem::ppn(0xF0000000));
        // } 

    }

    void per_core_init() {
        // Debug::printf("IN PER CORE INIT\n");
        // Debug::printf("CREATED PD\n");
        vmm_on(global_page_directory);
        // Debug::printf("TURNED ON VM\n");
    }

    void naive_munmap(void* p_, bool user) {
        // if((uint32_t)p_ < 0xF0000000) { // NOT SHARED
        //     auto me = impl::threads::state.current();
        //     ASSERT(!(me == nullptr));
        //     ASSERT(!(me->vme == nullptr));
        //     me->vme->remove_entry((uint32_t)p_, user);
        // }
        munmap(p_, user, PhysMem::FRAME_SIZE);
    }

 
    // DO NOT DELETE FOR MAPPING AT A REGION
    void* naive_mmap_location(uint32_t sz_, bool shared, StrongPtr<Node> node, uint32_t offset_, bool user, uint32_t location) {
        if(!shared) {
            auto me = impl::threads::state.current();
            ASSERT(!(me->vme == nullptr));
            return (void*)me->vme->add_entry_location(sz_, node, offset_, user, location);
        } 
        return (void*)shared_vme->add_entry_location(sz_, node, offset_, user, location);
    }

    // ORIGINAL NAIVE_MMAP DO NOT DELETE
    void* naive_mmap1(uint32_t sz_, bool shared, StrongPtr<Node> node, uint32_t offset_, bool user) {
        if(!shared) {
            auto me = impl::threads::state.current();
            ASSERT(!(me->vme == nullptr));
            return (void*)me->vme->add_entry(sz_, node, offset_, user);
        } 
        // Debug::printf("here\n");
        return (void*)shared_vme->add_entry(sz_, node, offset_, user);
    }

    // THIS IS THE MMAP FOR LINUX BUT WE ARE NOT USING YOU CAN DELETE IF YOU WANT
    // void* mmap(uint32_t* location, uint32_t size, uint32_t permissions, uint32_t flags, uint32_t fd, uint32_t offset) {
    //     void* mapped_region = nullptr;
    //     auto me = impl::threads::state.current();
    //     if(location) {
    //         if(flags & SHARED) {
    //             // normal shared mapping and shared anon mapping
    //             mapped_region = flags & ANON ? naive_mmap_location(size, true, (StrongPtr<Node>)nullptr, offset, true, (uint32_t)location) : 
    //                 naive_mmap_location(size, true, me->pcb->fdt[fd]->node, offset, true, (uint32_t)location);
    //         } else {
    //             // private mapping and private anon mapping
    //             mapped_region = flags & ANON ? naive_mmap_location(size, false, (StrongPtr<Node>)nullptr, offset, true, (uint32_t)location) : 
    //                 naive_mmap_location(size, false, me->pcb->fdt[fd]->node, offset, true, (uint32_t)location);
    //         }
    //     } else {
    //         if(flags & SHARED) {
    //             // normal shared mapping and shared anon mapping
    //             mapped_region = flags & ANON ? naive_mmap1(size, true, (StrongPtr<Node>)nullptr, offset, true) : 
    //                 naive_mmap1(size, true, me->pcb->fdt[fd]->node, offset, true);
    //         } else {
    //             // private mapping and private anon mapping
    //             mapped_region = flags & ANON ? naive_mmap1(size, false, (StrongPtr<Node>)nullptr, offset, true) : 
    //                 naive_mmap1(size, false, me->pcb->fdt[fd]->node, offset, true);
    //         }
    //     }
    //     // set premissions
    //     return mapped_region;
    // }

    // OUR MMAP IMPLEMENTATION ABSOLUTELY DO NOT DELETE
    void* mmap1(uint32_t* location, uint32_t size, uint32_t permissions, uint32_t flags, StrongPtr<Node> file, uint32_t offset) {
        void* mapped_region = nullptr;
        // auto me = impl::threads::state.current()
        Debug::printf("location: %x\n", location);
        if(location) {
            if(flags & SHARED) {
                // normal shared mapping and shared anon mapping
                mapped_region = flags & ANON ? naive_mmap_location(size, true, (StrongPtr<Node>)nullptr, offset, true, (uint32_t)location) : 
                    naive_mmap_location(size, true, file, offset, true, (uint32_t)location);
                    Debug::printf("size, %d, permissions %x, flags %x, fd %x, offset %x\n", size, permissions, flags, offset);
                    if(!(flags & ANON)) Debug::printf("file %x\n", file);
            } else {
                // private mapping and private anon mapping
                mapped_region = flags & ANON ? naive_mmap_location(size, false, (StrongPtr<Node>)nullptr, offset, true, (uint32_t)location) : 
                    naive_mmap_location(size, false, file, offset, true, (uint32_t)location);
                    Debug::printf("mmaped w/location\n");
            }
        } else {
            if(flags & SHARED) {
                // normal shared mapping and shared anon mapping
                Debug::printf("shared w/out location\n");
                Debug::printf("anon? %x\n", flags&ANON);
                mapped_region = flags & ANON ? naive_mmap1(size, true, (StrongPtr<Node>)nullptr, offset, true) : 
                    naive_mmap1(size, true, file, offset, true);
            } else {
                // private mapping and private anon mapping
                Debug::printf("anon/private w/out location\n");
                mapped_region = flags & ANON ? naive_mmap1(size, false, (StrongPtr<Node>)nullptr, offset, true) : 
                    naive_mmap1(size, false, file, offset, true);
                    Debug::printf("mmaped w/out location\n");
            }
        }
        // set premissions
        return mapped_region;
    }
    void* naive_mmap(uint32_t sz_, bool shared, StrongPtr<Node> node, uint32_t offset_, bool user) { // this is for forcing terrance's code to call mmap
        // Debug::printf("size %x, shared %x, node %x, offset %x, user %x\n", sz_, shared, node, offset_, user);
        return  mmap1(nullptr, sz_, 0, shared, node, offset_ );
        // return naive_mmap1(sz_, shared, node, offset_, user);
    }

    void munmap(void* p_, bool user, uint32_t size) {
        auto me = impl::threads::state.current();
        ASSERT(me != nullptr);

        uint32_t va = (uint32_t)p_;
        uint32_t num_pages = (size + PhysMem::FRAME_SIZE - 1) / PhysMem::FRAME_SIZE;

        if (va >= 0xF0000000) {
            shared_vme_lock->lock(); 
            shared_vme->remove_mapped_region(va, size, false); 

            shared_vme_lock->unlock();
        } else {
            if (me->vme != nullptr) {
                me->vme->remove_mapped_region(va, size, user);
            }
        }

        for (uint32_t i = 0; i < num_pages; i++) {
            uint32_t page_addr = va + i * PhysMem::FRAME_SIZE;
            remove_mapping((uint32_t*)getCR3(), page_addr >> 12);
        }
    }


} /* namespace vmm */

extern "C" void vmm_pageFault(uintptr_t va_, uintptr_t *saveState) {
    StrongPtr<VMEEntry> vme_entry;
    bool unlock = false;
    // Debug::printf("Page fault at VA: %x\n", va_);
    // Debug::printf("Trying to look up VME entry...\n");

    if(va_ >= 0xF0000000) { // SHARED
        unlock = true;
        VMM::shared_vme_lock->lock();
        if(VMM::check_VPN((uint32_t*)(getCR3()), va_ >> 12)) { // idk why this is here, checks if vpn is present and exits
            VMM::shared_vme_lock->unlock();
            return;
        }
        vme_entry = VMM::shared_vme->get(va_);
    } else if(va_ >= 0x80000000) { // NOT SHARED
        auto me = impl::threads::state.current();
        ASSERT(me != nullptr && !(me->vme == nullptr));
        vme_entry = me->vme->get(va_);
    }
    // if(vme_entry == nullptr) Debug::panic("*** can't handle page fault at %x\n",va_);
    if(vme_entry == nullptr) {// what to do here
        Debug::printf("VME ENTRY IS NULL\n");
        // couldve gotten here because a shared mapping was trying to be accessed
        exit(-1);
    }
    
    uint32_t new_page = PhysMem::alloc_frame();//pa
    // Debug::printf("vme_entry: %x\n", vme_entry);
    // if(chunk_size > PhysMem::FRAME_SIZE) chunk_size = PhysMem::FRAME_SIZE;
    VMM::add_mapping((uint32_t*)(getCR3()), (va_ >> 12), new_page >> 12, false, true, true);
    if(unlock) VMM::shared_vme_lock->unlock();
    
    if(!(vme_entry->file == nullptr)) {
        
        uint32_t page_start = va_ & ~0xFFF;
        uint32_t page_offset = (page_start - vme_entry->start);
        uint32_t size = vme_entry->size - page_offset;
        if(size > PhysMem::FRAME_SIZE) size = PhysMem::FRAME_SIZE;
        
        
        // vme_entry->file->read_all(vme_entry->file_offset + page_offset, size, (char*)page_start);
        vme_entry->file->read_all(vme_entry->file_offset + page_offset, size, (char*)new_page);
        // Debug::printf("HERE!\n");

        // Debug::printf("IN PAGE FAULT HANDLER | PA: %x\n", new_page);
        // Debug::printf("IN PAGE FAULT HANDLER | r_PA: %s\n", (char*)(new_page));
        // Debug::printf("IN PAGE FAULT HANDLER | r_VA: %s\n", (char*)(page_start));
        // Debug::printf("IN PAGE FAULT HANDLER | r_BUFFER: %s\n", (char*)(buffer));
        PageCache::add_page(va_, new_page, vme_entry->permissions, vme_entry->flags, vme_entry->file, vme_entry->file_offset);
    }
    // Debug::printf("VA: %x | NEW PAGE: %x\n", va_, new_page);
    
    // PageCache::add_page(va_, new_page, vme_entry->permissions, vme_entry->flags, vme_entry->file, vme_entry->file_offset);
    return;
    // memcpy((void*)new_page, (void*)(vme_entry->start + page_offset), PhysMem::FRAME_SIZE);
    // Debug::panic("*** can't handle page fault at %x\n",va_);
}
