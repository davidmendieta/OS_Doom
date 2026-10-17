//! TERRIBLE IMPLEMENTATION - IT WORKS SO DON'T WORRY ABOUT IT

#include "vme.h"
#include "physmem.h"
#include "ext2.h"

template class VME<NoLock>;
template class VME<BlockingLock>;

template void VME<NoLock>::insert_entry_sorted(uint32_t, uint32_t, uint32_t, StrongPtr<Node>, uint32_t, bool);
template void VME<NoLock>::coallescing();
template void VME<NoLock>::insert_free_space(uint32_t, uint32_t);
template StrongPtr<VMEEntry> VME<NoLock>::get(uint32_t);
template uint32_t VME<NoLock>::add_entry(uint32_t, StrongPtr<Node>, uint32_t, bool);
template void VME<NoLock>::remove_entry(uint32_t, bool);
// template StrongPtr<VME<NoLock>> VME<NoLock>::duplicate(StrongPtr<VME<NoLock>>);

template void VME<BlockingLock>::insert_entry_sorted(uint32_t, uint32_t, uint32_t, StrongPtr<Node>, uint32_t, bool);
template void VME<BlockingLock>::coallescing();
template void VME<BlockingLock>::insert_free_space(uint32_t, uint32_t);
template StrongPtr<VMEEntry> VME<BlockingLock>::get(uint32_t);
template uint32_t VME<BlockingLock>::add_entry(uint32_t, StrongPtr<Node>, uint32_t, bool);
template void VME<BlockingLock>::remove_entry(uint32_t, bool);
// template StrongPtr<VME<BlockingLock>> VME<BlockingLock>::duplicate(StrongPtr<VME<BlockingLock>>);

VMEEntry::VMEEntry(uint32_t start, uint32_t num_pages, uint32_t size, StrongPtr<Node> file, uint32_t file_offset, bool user, uint32_t permissions, uint32_t flags): 
    start(start), num_pages(num_pages), size(size), file(file), file_offset(file_offset), user(user), permissions(permissions), flags(flags) {};
VMEEntry::~VMEEntry(){
    for(uint32_t i = 0; i < num_pages; i++) {
        VMM::remove_mapping((uint32_t*)getCR3(), ((uint32_t)start + i * PhysMem::FRAME_SIZE) >> 12);
    }
};

template <typename Lock>
VME<Lock>::VME(uint32_t start, uint32_t end) : avail(start), limit(end), lock() {};

template <typename Lock>
VME<Lock>::~VME() {
}


template <typename Lock>
void VME<Lock>::insert_entry_sorted(uint32_t start, uint32_t num_pages, uint32_t size,
                         StrongPtr<Node> file, uint32_t file_offset, bool user) {
    StrongPtr<VMEEntry> prev{nullptr};
    StrongPtr<VMEEntry> curr = entries;

    while (!(curr == nullptr) && curr->start < start) {
        //! LOOP HERE -  Debug::printf("HERE\n");
        prev = curr;
        curr = curr->next;
    }


    StrongPtr<VMEEntry> new_node =
        StrongPtr<VMEEntry>::make(start, num_pages, size, file, file_offset, user, 0, 0);

    new_node->next = curr;
    if (prev == nullptr) {
        entries = new_node;
    } else {
        prev->next = new_node;
    }
}

template <typename Lock>
void VME<Lock>::coallescing() {
    StrongPtr<FreeEntry> curr = free_entries;
    while (!(curr == nullptr) && !(curr->next == nullptr)) {
        if (curr->start + curr->num_pages * PhysMem::FRAME_SIZE ==
            curr->next->start) {
            curr->num_pages += curr->next->num_pages;
            curr->next = curr->next->next;
        } else {
            curr = curr->next;
        }
    }
}

template <typename Lock>
void VME<Lock>::insert_free_space(uint32_t start, uint32_t num_pages) {
    StrongPtr<FreeEntry> prev{nullptr};
    StrongPtr<FreeEntry> curr = free_entries;

    while (!(curr == nullptr) && curr->start < start) {
        prev = curr;
        curr = curr->next;
    }

    StrongPtr<FreeEntry> new_node =
        StrongPtr<FreeEntry>::make(start, num_pages);
    new_node->next = curr;
    if (prev == nullptr) {
        free_entries = new_node;
    } else {
        prev->next = new_node;
    }

    coallescing();
}


template <typename Lock>
StrongPtr<VMEEntry> VME<Lock>::get(uint32_t va) {
    LockGuard g{lock};
    StrongPtr<VMEEntry> curr = entries;
    while (!(curr == nullptr)) {
        if (va >= curr->start && va < curr->start + curr->num_pages * PhysMem::FRAME_SIZE) {
            return curr;
        }
        curr = curr->next;
    }
    return StrongPtr<VMEEntry>(); 
}

template <typename Lock>
uint32_t VME<Lock>::add_entry(uint32_t size, StrongPtr<Node> file, uint32_t file_offset, bool user) {
    LockGuard g{lock};
    // Debug::printf("in add_entry\n");

    StrongPtr<FreeEntry> prev{nullptr};
    StrongPtr<FreeEntry> curr = free_entries;

    uint32_t va = 0;

    
    uint32_t required_pages =
    (size + PhysMem::FRAME_SIZE - 1) / PhysMem::FRAME_SIZE;
    
    while (!(curr == nullptr)) {
        if (curr->num_pages >= required_pages) {
            va = curr->start;
            curr->start += required_pages * PhysMem::FRAME_SIZE;
            curr->num_pages -= required_pages;

            if (curr->num_pages == 0) {
                if (prev == nullptr) {
                    free_entries = curr->next;
                } else {
                    prev->next = curr->next;
                }
            }
            break;
        }
        prev = curr;
        curr = curr->next;
    }

    
    if (va == 0) {
        if (avail + size > limit) return 0;  //! NO MORE SPACE
        va = avail;
        avail += PhysMem::FRAME_SIZE * required_pages;
    }
    
    // Debug::printf("in add_entry\nvaddr:%x filesz:%x fileoff:%x\n", va, file, file_offset);
    insert_entry_sorted(va, required_pages, size, file, file_offset, user);
    return va;
}

template<typename Lock>
void VME<Lock>::remove_mapped_region(uint32_t va, uint32_t size, bool user) {
    LockGuard g{lock};
    uint32_t Ustart = va; // start addr of what to unmap
    uint32_t Uend   = va + size; // end addr of what to unmap
    StrongPtr<VMEEntry> prev{nullptr};
    StrongPtr<VMEEntry> curr = entries;

    while (curr != nullptr) {
        uint32_t Estart = curr->start; // start addr of current vme
        uint32_t Eend   = curr->start + curr->size; // end addr of current vme

        // if no overlap, just advance
        if (Uend <= Estart || Ustart >= Eend) {
            prev = curr;
            curr = curr->next;
            continue;
        }

        // case 1: U completely covers E ---> remove this entry
        if (Ustart <= Estart && Uend >= Eend) {
            auto to_remove = curr;
            if (prev == nullptr) entries = curr->next;
            else prev->next = curr->next;
            curr = curr->next;                 // advance before we delete
            insert_free_space(Estart, to_remove->num_pages);
            // keep prev the same, keep unmap window [Ustart,Uend) as-is
            continue;
        }

        // case 2: U covers the front chunk of E ---> trim head
        if (Ustart <= Estart && Uend < Eend) {
            uint32_t new_start = Uend;
            uint32_t new_size  = Eend - Uend;
            curr->start = new_start;
            curr->size  = new_size;
            curr->num_pages = (new_size + PhysMem::FRAME_SIZE-1)/PhysMem::FRAME_SIZE;
            insert_free_space(Estart, (Uend - Estart + PhysMem::FRAME_SIZE-1)/PhysMem::FRAME_SIZE);
            // done with this entry; unmap window fully consumed here
            break;
        }

        // case 3: U covers the back chunk of E ---> trim tail
        if (Ustart > Estart && Uend >= Eend) {
            uint32_t keep_size = Ustart - Estart;
            insert_free_space(Ustart, (Eend - Ustart + PhysMem::FRAME_SIZE-1)/PhysMem::FRAME_SIZE);
            curr->size = keep_size;
            curr->num_pages = (keep_size + PhysMem::FRAME_SIZE-1)/PhysMem::FRAME_SIZE;
            // we've consumed [Estart, Eend) from our unmap window
            // shrink the unmap window so next iteration only hits what's left
            Ustart = Eend;
            curr = curr->next;
            continue;
        }

        // case 4: U slices through the middle ---> split
        // Estart < Ustart < Uend < Eend
        {
            uint32_t left_size  = Ustart - Estart;
            uint32_t right_size = Eend - Uend;
            // 1) adjust the current entry to become the "left" piece
            curr->size      = left_size;
            curr->num_pages = (left_size + PhysMem::FRAME_SIZE - 1)/PhysMem::FRAME_SIZE;
            // 2) allocate a brand-new entry for the right piece
            auto right = StrongPtr<VMEEntry>::make(
                Uend,
                (right_size + PhysMem::FRAME_SIZE - 1)/PhysMem::FRAME_SIZE,
                right_size,
                curr->file, curr->file_offset + (Uend - Estart),
                curr->user, curr->permissions, curr->flags);
            // link it in
            right->next = curr->next;
            curr->next  = right;
            // free the middle chunk
            insert_free_space(Ustart, (Uend - Ustart + PhysMem::FRAME_SIZE-1)/PhysMem::FRAME_SIZE);
            break;
        }
    }
}

// template <typename Lock>
// uint32_t VME<Lock>::add_entry_location(uint32_t size, StrongPtr<Node> file, uint32_t file_offset, bool user, uint32_t location) {
//     LockGuard g{lock};
//     StrongPtr<FreeEntry> prev{nullptr};
//     StrongPtr<FreeEntry> curr = free_entries;

//     uint32_t va = 0;

    
//     uint32_t required_pages =
//     (size + PhysMem::FRAME_SIZE - 1) / PhysMem::FRAME_SIZE;
    
//     while (!(curr == nullptr) && curr->start >= location) {
//         if (curr->num_pages >= required_pages) {
//             va = curr->start;
//             curr->start += required_pages * PhysMem::FRAME_SIZE;
//             curr->num_pages -= required_pages;

//             if (curr->num_pages == 0) {
//                 if (prev == nullptr) {
//                     free_entries = curr->next;
//                 } else {
//                     prev->next = curr->next;
//                 }
//             }
//             break;
//         }
//         prev = curr;
//         curr = curr->next;
//     }

    
//     if (va == 0) {
//         if (avail + size > limit) return 0;  //! NO MORE SPACE
//         va = avail;
//         avail += PhysMem::FRAME_SIZE * required_pages;
//     }
    
//     insert_entry_sorted(va, required_pages, size, file, file_offset, user);
//     return va;
// }
template <typename Lock>
uint32_t VME<Lock>::add_entry_location(uint32_t size, StrongPtr<Node> file, uint32_t file_offset, bool user, uint32_t location) {
    LockGuard g{lock};
    StrongPtr<FreeEntry> prev{nullptr};
    StrongPtr<FreeEntry> curr = free_entries;

    uint32_t va = 0;
    
    
    uint32_t required_pages =
    (size + PhysMem::FRAME_SIZE - 1) / PhysMem::FRAME_SIZE;

    while (!(curr == nullptr) && curr->start >= location) {
        if (curr->num_pages >= required_pages) {
            va = curr->start;
            curr->start += required_pages * PhysMem::FRAME_SIZE;
            curr->num_pages -= required_pages;

            if (curr->num_pages == 0) {
                if (prev == nullptr) {
                    free_entries = curr->next;
                } else {
                    prev->next = curr->next;
                }
            }
            break;
        }
        prev = curr;
        curr = curr->next;
    }

    if (va == 0) {
        if (avail + size > limit) return 0;  //! NO MORE SPACE
        va = avail;
        avail += PhysMem::FRAME_SIZE * required_pages;
    }

    insert_entry_sorted(va, required_pages, size, file, file_offset, user);
    return va;
}


template <typename Lock>
void VME<Lock>::remove_entry(uint32_t va, bool user) {
    LockGuard g{lock};
    StrongPtr<VMEEntry> prev{nullptr};
    StrongPtr<VMEEntry> curr = entries;

    while (!(curr == nullptr)) {
        if (va >= curr->start && va < curr->start + curr->num_pages * PhysMem::FRAME_SIZE && (!user || curr->user == user)) {
            uint32_t start = curr->start;
            uint32_t size = curr->num_pages;

            if (prev == nullptr) {
                entries = curr->next;
            } else {
                prev->next = curr->next;
            }

            insert_free_space(start, size);
            return;
        }
        prev = curr;
        curr = curr->next;
    }
    return;
}

template <typename Lock>
StrongPtr<VME<Lock>> VME<Lock>::duplicate(StrongPtr<VME<Lock>> vme) {
    StrongPtr<VME<Lock>> new_vme = StrongPtr<VME<Lock>>::make(vme->avail, vme->limit);
    new_vme->entries = VMEEntry::duplicate(vme->entries);
    new_vme->free_entries = FreeEntry::duplicate(vme->free_entries);
    return new_vme;
}

StrongPtr<FreeEntry> FreeEntry::duplicate(StrongPtr<FreeEntry> free_entry) {
    if(free_entry == nullptr) return StrongPtr<FreeEntry>();
    StrongPtr<FreeEntry> new_free_entry = StrongPtr<FreeEntry>::make(free_entry->start, free_entry->num_pages);
    new_free_entry->next = FreeEntry::duplicate(free_entry->next);
    return new_free_entry;
}

StrongPtr<VMEEntry> VMEEntry::duplicate(StrongPtr<VMEEntry> vme_entry) {
    if(vme_entry == nullptr) return StrongPtr<VMEEntry>();
    // Debug::printf("HERE\n");
    // Debug::printf("START: %x | NUM_PAGES: %d | SIZE: %d | FILE: %d | OFFSET: %d\n", vme_entry->start, vme_entry->num_pages, vme_entry->size, vme_entry->file, vme_entry->file_offset);
    StrongPtr<VMEEntry> new_vme_entry = StrongPtr<VMEEntry>::make(vme_entry->start, vme_entry->num_pages, 
        vme_entry->size, vme_entry->file, vme_entry->file_offset, vme_entry->user, vme_entry->permissions, vme_entry->flags);
    // Debug::printf("AFTER CREATION\n");
    new_vme_entry->next = VMEEntry::duplicate(vme_entry->next);
    return new_vme_entry;
}


