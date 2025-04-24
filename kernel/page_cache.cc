#include "shared.h"
#include "blocking_lock.h"
#include "physmem.h"
#include "queue.h"


namespace page_cache { 
  
   uint32_t hashtable [1024]; // the


   void cache_init() { // call init for page cache??
       for (uint32_t i = 0; i < 1024; i++) {
           hashtable[i] = 0;
       }


   }


   uint32_t hash(uint32_t va) {
       // return (fs + inum + block_num) % 1024;
       return va % 1024;
   }


   uint32_t evict_page() { // do we need to work with tlb?
       for(uint32_t revolutions = 0; revolutions <= 2; revolutions++) {// when page_cache runs out of space
           for(uint32_t i = 0; i < 1024; i++) {
               if(!(hashtable[i] & 0x20)) {
                   hashtable[i] = 0;
                   //write to disk
                   // VMM::naive_munmap((uint32_t*)hashtable[i], true); // free up space in PD. Do we need to do this?
                   return i;
               }
               hashtable[i] &= ~0x10;
           }
       }
       return -1;
   }
  
  
   void evict_page(uint32_t va) { // for the system call to remove a page
       uint32_t hash_val = hash(va);


       if (hashtable[hash_val] != 0) {
           if (hashtable[hash_val] & 0x2) {
               // write page back to disk we need write
           }


           // free physmem
           if (hashtable[hash_val] != 0) {
               // PhysMem::dealloc_frame?
           }


           // clear entry
           hashtable[hash_val] = 0;
       }
   }


   void flush_page(uint32_t va) {
       uint32_t hash_val = hash(va);


       // check if page exists in thecache
       if (hashtable[hash_val] != 0) {
           // page dirty so write back
           if (hashtable[hash_val] & 0x2) {
               // we need actual file system write operations to write this page back to the disk lol
               hashtable[hash_val] &= ~0x2; // clear dirty bit after flush
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
           hashtable[hash_val] &= ~0x10;
       }


   }
  
   // void preload(/*needs params*/);
   //idk how to do this one
  
   void flush_all() {
       for (uint32_t i = 0; i < 1024; i++) {
           if (hashtable[i] != 0 && (hashtable[i] & 0x2)) {
               // write page back to disk
               // once again we need filesystem write oepraitons to do this


               hashtable[i] &= ~0x2; // clear dirty bit after flush
           }
       }
   }
  
   // uint32_t copy_on_write(uint32_t /*some addr here*/);
   // idk how to do this one


}



