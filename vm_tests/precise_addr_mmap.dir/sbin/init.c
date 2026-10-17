#include "libc.h"

#define NONE  0x0
#define READ  0x1
#define WRITE 0x2
#define EXEC  0x4

#define SHARED     0x01
#define PRIVATE    0x02
#define FIXED      0x10
#define ANON       0x20


extern void* naive_mmap_location(uint32_t size, int is_shared, int file, uint32_t offset, int user, uint32_t location);

int main(int argc, char** argv) {
    printf("*** Starting mmap test\n");
    
    int test_fd = open("/hello", 0);
    if (test_fd < 0) {
        printf("*** Error: Failed to open test file\n");
        shutdown();
        return -1;
    }
    int fileSize = len(test_fd);
    printf("*** Testing size: %d bytes\n", fileSize);
    
    // regular mapping
    void* regular_map = (void*)naive_mmap(fileSize, 1, test_fd, 0);
    if (regular_map == 0) {
        printf("*** Error: Regular mapping failed\n");
        close(test_fd);
        shutdown();
        return -1;
    }
    
    printf("*** Regular mapping succeeded at: 0x%x\n", (unsigned int)regular_map);
    
    // write to mapping ot see if it works
    char* p = (char*)regular_map;
    printf("*** Content in regular mapping: %s\n", p);
    
    // use munmap to clean up
    naive_munmap(regular_map);
    
    // Now we try to test mapping with a specific address using the mmap syscall
    // Try to map at address 0xfee01000 in the shared region
    uint32_t target_addr = 0xfee01000;
    printf("*** Trying to map at specific address: 0x%lx\n", target_addr);
    
    void* fixed_map = (void*)mmap(target_addr, fileSize, READ, SHARED | FIXED, test_fd, 0);
    if (fixed_map == 0) {
        printf("*** Error: Fixed mapping failed\n");
        close(test_fd);
        shutdown();
        return -1;
    }
    
    printf("*** Fixed mapping succeeded at: 0x%x\n", (unsigned int)fixed_map);
    
    // check if we got the address we requested
    if ((unsigned int)fixed_map == target_addr) {
        printf("*** Got the exact address we requested!\n");
    } else {
        printf("*** Did not get requested address\n");
    }
    
    // read from the fixed mapping
    p = (char*)fixed_map;
    printf("*** Content in fixed mapping: %s\n", p);
    
    // clean up
    munmap(fixed_map, true, fileSize);
    close(test_fd);
    
    printf("*** mmap test completed successfully\n");
    shutdown();
    return 0;
}
