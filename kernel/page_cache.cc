#include "shared.h"
#include "blocking_lock.h"
#include "physmem.h"
#include "queue.h"

namespace page_cache {  
    
    uint32_t hashtable [1024];

    void cache_init() { // call init for page cache?? 

    }

    uint32_t hash(uint32_t fs, uint32_t inum, uint32_t block_num) {
        return (fs + inum + block_num) % 1024;
    }
        
    uint32_t evict_page() { // for when we run out of room in page_cache
        bool no_eviction = false;
        uint32_t i = 0;
        while(no_eviction) {
            uint32_t entry = hashtable[i];
            if(entry & 0x10) {
                hashtable[i] = 0;
                //we dont have write to disk yet
            }
        }   
    }
    
    void evict_page(uint32_t va) { // for the system call to remove a page
        
    }

    void flush_page(uint32_t va) {

    }

    uint32_t get_page(uint32_t va) {

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