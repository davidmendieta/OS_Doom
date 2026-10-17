// #include "shared.h"
// #include "blocking_lock.h"
// #include "physmem.h"
// #include "queue.h"
#include "ext2.h"
#include "page_cache.h"

#define PG_CACHE_SIZE 4093
    namespace PageCache {
        Cache* global_cache = nullptr;
        Cache::Cache(uint32_t size) : cache_size(size) {
        for (uint32_t i = 0; i < PG_CACHE_SIZE; ++i) {
            entries[i] = nullptr;
        }
        // Initialization remains opt-in until cache identity/writeback are repaired.
        clock_hand = 0;
    }

    
    
    // uint32_t hashtable [1024]; // the 

// namespace PageCac
  
   uint32_t hashtable [1024]; // the


    // Thomas Wang's 32-bit Integer Hash https://gist.github.com/badboy/6267743
    uint32_t hash(uint32_t key) { 
        // return (fs + inum + block_num) % 1024;
        key = ~key + (key << 15);  // key = (key << 15) - key - 1;
        key = key ^ (key >> 12);
        key = key + (key << 2);
        key = key ^ (key >> 4);
        key = key * 2057;          // key = (key + (key << 3)) + (key << 11);
        key = key ^ (key >> 16);
        return  (key >> 12) % PG_CACHE_SIZE;
    }

    // CacheEntry* lookup_page (uint32_t va) {//, StrongPtr<Node> file, uint32_t offset) {
    //     uint32_t index = hash(va);
    //     Debug::printf("inside look up\n");
    //     for(uint32_t i = 0; i < PG_CACHE_SIZE; i++) {
    //         uint32_t probe = (index + i) % PG_CACHE_SIZE;
    //         CacheEntry* entry = global_cache->entries[probe];
    //         // Debug::printf("probe: %x\n", probe);
    //         // Debug::printf("entry: %x\n", entry);
    //         if (entry == nullptr) {
    //             Debug::printf("skip\n");
    //             continue; // or break if you know empty slots mean "stop"

    //         }
    
    //         if(entry) {
    //             Debug::printf("not null!!\n");
    //         }
    //         if(entry && entry->virtual_addr == va) {
    //             Debug::printf("hit]\n");
    //             return entry;
    //         }
    //     }
    //     Debug::printf("miss]\n");
    //     return nullptr;
    // }

    uint32_t normalize_vpn(uint32_t vpn) {
        return vpn & ~0x20; // clears access bit
    }    

    uint32_t evict_page(uint32_t va) {
        uint32_t vpn = va >> 12; // only work with VPN
        uint32_t index = hash(normalize_vpn(vpn)); // hash normalized VPN
        for (uint32_t revolutions = 0; revolutions <= 2; ++revolutions) {
            for (uint32_t i = 0; i < 8; ++i) {
                uint32_t probe = (index + i) % PG_CACHE_SIZE;
                CacheEntry* entry = global_cache->entries[probe];
                if (entry == nullptr) {
                    continue;
                }
                if (!(entry->vpn & 0x20)) { // Not recently accessed
                    Debug::printf("evict entry at %d\n", probe);
                    // delete entry; // Free memory
                    global_cache->entries[probe] = nullptr;
                    return probe;
                }
                // Clear access bit for next revolution
                entry->vpn &= ~0x20;
            }
        }
        Debug::printf("forced eviction at index 0\n");
        // delete global_cache->entries[0];
        global_cache->entries[index] = nullptr;
        return index;
    }
    uint32_t evict_page() {
        uint32_t index = global_cache->clock_hand;
        for (uint32_t revolutions = 0; revolutions <= 2; ++revolutions) {
            for (uint32_t i = 0; i < 8; ++i) {
                uint32_t probe = (index + i) % PG_CACHE_SIZE;
                CacheEntry* entry = global_cache->entries[probe];
                if (entry == nullptr) {
                    continue;
                }
                if (!(entry->vpn & 0x20)) { // Not recently accessed
                    Debug::printf("evict entry at %d\n", i);
                    // delete entry; // Free memory
                    global_cache->entries[probe] = nullptr;
                    global_cache->clock_hand = probe;
                    return probe;
                }
                // Clear access bit for next revolution
                entry->vpn &= ~0x20;
            }
        }
        Debug::printf("forced eviction at index 0\n");
        // delete global_cache->entries[0];
        global_cache->entries[index] = nullptr;
        return index;
    }
    

    void add_page(uint32_t va, uint32_t pa, uint32_t permissions, uint32_t flags, StrongPtr<Node> file, uint32_t offset, uint32_t size) {
        uint32_t vpn = va >> 12; // only work with VPN
        uint32_t index = hash(normalize_vpn(vpn)); // hash normalized VPN
        Debug::printf("adding vpn %x\n", vpn);
    
        for (uint32_t i = 0; i < 8; ++i) {
            uint32_t probe = (index + i) % PG_CACHE_SIZE;
            
            Debug::printf("probe: %x\n", probe);
            Debug::printf("entry: %x\n", global_cache->entries[probe]);
            
            if (global_cache->entries[probe] == nullptr) {
                Debug::printf("pass\n");
                CacheEntry* curr_entry = new CacheEntry;
                Debug::printf("wa\n");
                curr_entry->accessed = true;
                curr_entry->virtual_addr = va;
                curr_entry->file = file;
                curr_entry->dirty = false;
                curr_entry->file_offset = offset;
                curr_entry->flags = flags;
                curr_entry->permissions = permissions;
                curr_entry->physical_addr = pa;
                curr_entry->size = size;
                curr_entry->pg_cache_location = probe;
                curr_entry->vpn = vpn | 0x20; // set access bit initially
                global_cache->entries[probe] = curr_entry;
                Debug::printf("normal add at probe %x\n", probe);
                return;
            }
        }
    
        uint32_t evicted_index = evict_page(va);
        CacheEntry* curr_entry = new CacheEntry;
        curr_entry->accessed = true;
        curr_entry->virtual_addr = va;
        curr_entry->file = file;
        curr_entry->dirty = false;
        curr_entry->file_offset = offset;
        curr_entry->flags = flags;
        curr_entry->permissions = permissions;
        curr_entry->physical_addr = pa;
        curr_entry->size = size;
        curr_entry->pg_cache_location = evicted_index;
        curr_entry->vpn = vpn | 0x20; // set access bit initially
        global_cache->entries[evicted_index] = curr_entry;
        Debug::printf("forced add at probe %x\n", evicted_index);
    }

    CacheEntry* lookup_page(uint32_t va) {
        uint32_t vpn = va >> 12;               
        uint32_t index = hash(normalize_vpn(vpn));
    
        for (uint32_t i = 0; i < PG_CACHE_SIZE; ++i) {
            uint32_t probe = (index + i) % PG_CACHE_SIZE;
            CacheEntry* entry = global_cache->entries[probe];
    
            if (entry == nullptr) {
                // No entry, stop search early
                return nullptr;
            }
    
            if ((entry->vpn & ~0x20) == (vpn & ~0x20)) {
                // Found matching page
                entry->vpn |= 0x20; // Mark as accessed
                return entry;
            }
        }
    
        // Miss
        return nullptr;
    }
    
   void flush_page(uint32_t va) {
       uint32_t hash_val = hash(va);


       // check if page exists in thecache
       if (hashtable[hash_val] != 0) {
           // page dirty so write back
           if (hashtable[hash_val] & 0x20) {
               // we need actual file system write operations to write this page back to the disk lol
               hashtable[hash_val] &= ~0x20; // clear dirty bit after flush
           }
       }


   }


   uint32_t get_page(uint32_t va) {
       uint32_t hash_val = hash(va);
       return hashtable[hash_val];
   }
  
   void release_page(uint32_t va) {
       uint32_t hash_val = hash(va);


       if (hashtable[hash_val] != 0) {
           hashtable[hash_val] &= ~0x20;
       }


   }
  
   // void preload(/*needs params*/);
   //idk how to do this one
  
   void flush_all() {
       for (uint32_t i = 0; i < 1024; i++) {
           if (hashtable[i] != 0 && (hashtable[i] & 0x20)) {
               // write page back to disk
               // once again we need filesystem write oepraitons to do this


               hashtable[i] &= ~0x20; // clear dirty bit after flush
           }
       }
   }
  
   // uint32_t copy_on_write(uint32_t /*some addr here*/);
   // idk how to do this one


}

    // }

