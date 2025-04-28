
#include "libc.h"

#define NONE  0x0
#define READ  0x1
#define WRITE 0x2
#define EXEC  0x4

#define SHARED     0x01
#define PRIVATE    0x02
#define FIXED      0x10
#define ANON       0x20

int main(int argc, char** argv) {
    printf("*** Starting mmap test\n");
    
    //open file to mmpa
    int fd = open("/hello", 0);
    if (fd < 0) {
        printf("*** Error: Failed to open file\n");
        shutdown();
        return -1;
    }
    
    // get fs
    int fileSize = len(fd);
    printf("*** File size: %d bytes\n", fileSize);
    
    // test
    void* naive_mapped = (void*)naive_mmap(fileSize, 1, fd, 0);
    if (naive_mapped == 0) {
        printf("*** Error: naive_mmap failed\n");
        close(fd);
        shutdown();
        return -1;
    }
    
    printf("*** naive_mmap succeeded at address: 0x%x\n", (unsigned int)naive_mapped);
    printf("*** Content from naive_mmap: %s\n", (char*)naive_mapped);
    
    // test munmap
    naive_munmap(naive_mapped);
    
    printf("*** mmap test completed successfully\n");
    
    close(fd);
    shutdown();
    return 0;
}
