#include "libc.h"

#define NONE  0x0
#define READ  0x1
#define WRITE 0x2
#define EXEC  0x4

#define SHARED     0x01
#define PRIVATE    0x02
#define FIXED      0x10
#define ANON       0x20

// test params
#define TEST_ITERATIONS 5
#define BUFFER_SIZE 100

int main(int argc, char** argv) {
    printf("*** Starting Page Cache Test ***\n");
    
    // test file we'll use for mapping
    int test_fd = open("/hello", 0);
    if (test_fd < 0) {
        printf("*** Error: Failed to open test file\n");
        shutdown();
        return -1;
    }
    
    int fileSize = len(test_fd);
    printf("*** Test file size: %d bytes\n", fileSize);
    
    // first test: basic file mapping
    printf("\n*** Test 1: Basic File Mapping ***\n");
    void* mapping1 = (void*)naive_mmap(fileSize, PRIVATE, test_fd, 0);
    
    if (mapping1 == 0) {
        printf("*** Error: Mapping failed\n");
        close(test_fd);
        shutdown();
        return -1;
    }
    
    printf("*** Mapping at: 0x%x\n", (unsigned int)mapping1);
    printf("*** Content from mapping: %.20s\n", (char*)mapping1);
    
    // private mapping
    if (fileSize > 5) {
        char* data = (char*)mapping1;
        
        // save og vals
        char orig0 = data[0];
        char orig1 = data[1];
        
        // modify mapping
        data[0] = 'X';
        data[1] = 'Y';
        
        printf("*** After modification: %.20s\n", data);
        
        // bring back og vals
        data[0] = orig0;
        data[1] = orig1;
    }
    
    // clean up test 1
    naive_munmap(mapping1);
    
    // 2nd test is repeated access to memory-mapped file
    printf("\n*** Test 2: Repeated access to memory-mapped file ***\n");
    void* mapping = (void*)naive_mmap(fileSize, PRIVATE, test_fd, 0);
    
    if (mapping == 0) {
        printf("*** Error: Mapping failed\n");
        close(test_fd);
        shutdown();
        return -1;
    }
    
    printf("*** Mapping created at: 0x%x\n", (unsigned int)mapping);
    
    // access maped file repeatedly to check page cache
    char buffer[BUFFER_SIZE];
    
    for (int i = 0; i < TEST_ITERATIONS; i++) {
        printf("*** Iteration %d: Reading from mapped file\n", i+1);
        
        // access diff parts of file
        int offset = (i * 3) % (fileSize > 15 ? 15 : fileSize);
        int len = (fileSize - offset > BUFFER_SIZE) ? BUFFER_SIZE : fileSize - offset;
        
        // copy data from mapped region
        memcpy(buffer, (char*)mapping + offset, len);
        buffer[len] = '\0';
        
        printf("***   Read %d bytes starting at offset %d: %.20s%s\n", 
               len, offset, buffer, (len > 20) ? "..." : "");
    }
    
    // 3rd test: page cache retention --unmapping and remapping
    printf("\n*** Test 3: Unmapping and remapping to test page cache retention ***\n");
    
    // unmap
    naive_munmap(mapping);
    printf("*** Unmapped the file\n");
    
    // remap
    printf("*** Remapping the file\n");
    mapping = (void*)naive_mmap(fileSize, PRIVATE, test_fd, 0);
    
    if (mapping == 0) {
        printf("*** Error: Remapping failed\n");
        close(test_fd);
        shutdown();
        return -1;
    }
    
    printf("*** Remapped at: 0x%x\n", (unsigned int)mapping);
    
    // read
    printf("*** Reading from remapped file: %.20s%s\n", 
           (char*)mapping, (fileSize > 20) ? "..." : "");
    
    // 4th test is multiple private mappings working correctly
    printf("\n*** Test 4: Multiple Private Mappings ***\n");
    
    // create more priv mappings
    void* private_map2 = (void*)naive_mmap(fileSize, PRIVATE, test_fd, 0);
    if (private_map2 == 0) {
        printf("*** Error: Second private mapping failed\n");
        naive_munmap(mapping);
        close(test_fd);
        shutdown();
        return -1;
    }
    
    printf("*** First mapping at: 0x%x\n", (unsigned int)mapping);
    printf("*** Second mapping at: 0x%x\n", (unsigned int)private_map2);
    
    printf("*** Content from first mapping: %.20s\n", (char*)mapping);
    printf("*** Content from second mapping: %.20s\n", (char*)private_map2);
    
    // modify first priv mapping
    if (fileSize > 5) {
        char* data1 = (char*)mapping;
        data1[0] = 'P';
        data1[1] = 'R';
        printf("*** After modifying first mapping: %.20s\n", data1);
        
        // make sure second mapping is unaffected (private mappings)
        printf("*** Second mapping content: %.20s\n", (char*)private_map2);
    }
    
    // clean this one up
    naive_munmap(private_map2);
    naive_munmap(mapping);
    close(test_fd);
    
    printf("\n*** Page Cache Test Completed Successfully ***\n");
    shutdown();
    return 0;
}
