#include "shared.h"
#include "blocking_lock.h"
#include "physmem.h"
#include "queue.h"

namespace page_cache {  

    void cache_init() { // call init for page cache?? 

    }

    uint32_t hash(uint32_t raw_va) {

    }
        
    void evict_page() { // for when we run out of room in page_cache

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