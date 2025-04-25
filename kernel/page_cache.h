#pragma once

#include "stdint.h"
#include "shared.h"
#include "blocking_lock.h"
#include "physmem.h"

namespace PageCache {

#define NONE  0x0       
#define READ  0x1       
#define WRITE 0x2       
#define EXEC  0x4       

#define SHARED     0x01  
#define PRIVATE    0x02  
#define FIXED      0x10  
#define ANON       0x20  

struct CacheEntry {
    uint32_t virtual_addr;    
    uint32_t physical_addr;   
    StrongPtr<Node> file;     
    uint32_t file_offset;     
    uint32_t flags;           
    uint32_t permissions;     
    bool dirty;               
    bool accessed;          
    uint32_t ref_count;
    uint32_t pg_cache_location;  
    uint32_t vpn;     

    ~CacheEntry();
};

class Cache {
private:
    CacheEntry* entry_table;
    uint32_t cache_size = 4093; 
    uint32_t clock_hand; // clock algo
    BlockingLock lock; // protect cache access

//internalm ethdos
    uint32_t hash(uint32_t va);
    uint32_t hash32(uint32_t key); // Thomas Wang's 32 bit Integer Hash
    int find_entry(uint32_t va);
    int evict_page();
    void mark_referenced(int index);

public:
    Cache(uint32_t size);
    ~Cache();

    // page cache methods imple. in page)cache.cc
    
    CacheEntry* entries[4093]; // arr of cache entries
    uint32_t get_page(uint32_t va, StrongPtr<Node> file, uint32_t file_offset, bool write_access);
    void release_page(uint32_t va);
    void flush_page(uint32_t va);
    void evict_page(uint32_t va);
    
    // uint32_t copy_on_write(uint32_t va);
    // void preload(uint32_t va, uint32_t size, StrongPtr<Node> file, uint32_t file_offset);
    void flush_all();
};

// namespace PageCache {
void add_page(uint32_t va, uint32_t pa, uint32_t permissions, uint32_t flags, StrongPtr<Node> file, uint32_t offset);
// CacheEntry* lookup_page(uint32_t va, StrongPtr<Node> file, uint32_t offset);
// global page cache instance
extern Cache* global_cache;

// init page cache w/ specific size
extern void init(uint32_t size);

}  
