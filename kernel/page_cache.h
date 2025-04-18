#pragma once

#include "stdint.h"
#include "shared.h"
#include "blocking_lock.h"
#include "physmem.h"
#include "queue.h"

// some stuff here
namespace page_cache {
    class Cache {
    private:
        // have hash table here
        uint32_t hashtable [1024];
        // le hash function
        uint32_t hash(uint32_t );
        
        void evict_page();
        
        void flush_page(/*something here*/);

    public:
        Cache(uint32_t max_cache_size);
        ~Cache();
        
        uint32_t get_page(/*needs params*/);
        
        void release_page(uint32_t /*some addr here*/);
        
        void preload(/*needs params*/);
        
        void flush_all();
        
        uint32_t copy_on_write(uint32_t /*some addr here*/);
    };
}