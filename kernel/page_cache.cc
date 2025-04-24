// #include "shared.h"
// #include "blocking_lock.h"
// #include "physmem.h"
// #include "queue.h"
#include "ext2.h"
#include "page_cache.h"

#define PG_CACHE_SIZE 4093
    namespace PageCache {
        Cache* global_cache = nullptr;
        Cache::Cache(uint32_t size) : cache_size(size), clock_hand(0) {
        for (uint32_t i = 0; i < PG_CACHE_SIZE; ++i) {
            entries[i] = nullptr;
        }
        global_cache = new Cache(4093);
    }

    
    
    // uint32_t hashtable [1024]; // the 

    void cache_init() { // call init for page cache?? 
        
    }


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

    uint32_t evict_page() { // do we need to work with tlb?
        for(uint32_t revolutions = 0; revolutions <= 2; revolutions++) {// when page_cache runs out of space
            for(uint32_t i = 0; i < 1024; i++) {
                if(!(global_cache->entries[i]->vpn & 0x20)) {
                    // hashtable[i] = 0;
                    //write to disk
                    global_cache->entries[i] = nullptr;
                    return i;
                }
                // hashtable[i] &= ~0x10;
                global_cache->entries[i]->vpn &= ~0x20;
            }
        }
        global_cache->entries[0] = nullptr;
        return 0;
    }
    
    void evict_page(uint32_t va) { // for the system call to remove a page
        
    }

    void add_page(uint32_t va, uint32_t pa, uint32_t permissions, uint32_t flags, StrongPtr<Node> file, uint32_t offset) {
        uint32_t index = hash(va);
        for(uint32_t i = 0; i < PG_CACHE_SIZE; ++i) {// idk chat said this is more efficent and doesn't change anything
            uint32_t probe = (index + i) % PG_CACHE_SIZE;
            auto curr_entry = global_cache->entries[i];
            if (curr_entry == nullptr) {
                curr_entry = new CacheEntry;
                curr_entry->accessed = true;
                curr_entry->virtual_addr = va;
                curr_entry->file = file;
                curr_entry->dirty = false;
                curr_entry->file_offset = offset;
                curr_entry->flags = flags;
                curr_entry->permissions = permissions;
                curr_entry->physical_addr = pa;
                curr_entry->pg_cache_location = probe;
                curr_entry->vpn = (curr_entry->virtual_addr >> 12) | 0x20;
                global_cache->entries[i] = curr_entry;
            }
        }
        // didnt find a location need to evict
        uint32_t evicted_page_index = evict_page();
        if(global_cache->entries[evicted_page_index] == nullptr) {
            auto curr_entry = global_cache->entries[evicted_page_index];
            curr_entry = new CacheEntry;
            curr_entry->accessed = true;
            curr_entry->virtual_addr = va;
            curr_entry->file = file;
            curr_entry->dirty = false;
            curr_entry->file_offset = offset;
            curr_entry->flags = flags;
            curr_entry->permissions = permissions;
            curr_entry->physical_addr = pa;
            curr_entry->pg_cache_location = evicted_page_index;
            global_cache->entries[evicted_page_index] = curr_entry;
        }
    }


    void flush_page(uint32_t va) {

    }

    uint32_t get_page(uint32_t va) {
        uint32_t hash_val = hash(va);
        return global_cache->entries[hash_val]->virtual_addr;//wrong
    }
    
    void release_page(uint32_t va) {

    }
    
    // void preload(/*needs params*/);
    //idk how to do this one
    
    void flush_all() {

    }
    
    // uint32_t copy_on_write(uint32_t /*some addr here*/);
    // idk how to do this one
}