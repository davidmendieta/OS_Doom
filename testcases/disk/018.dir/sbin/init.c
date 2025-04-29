
#include "sys.h"
#include "libc.h"
#include <stddef.h> // for size_t

#define BUFFER_SIZE 8  
#define TEST_STRING "The quick brown fox jumps over the lazy dog."


void print_file(int fd, int size){
    seek(fd, 0);
    char result[size + 1];
    int n = read(fd, result, size);
    result[n] = 0;
    printf("file content: %s", result);
}

// set each byte to the specified value
void *my_memset(void *ptr, int value, size_t num) {
    unsigned char *p = (unsigned char *)ptr; // cast to unsigned char pointer
    for (size_t i = 0; i < num; i++) {
        p[i] = (unsigned char)value; 
    }
    return ptr; 
}


int main() {
    printf("=== Testing truncate functionality ===\n");

    uint32_t O_RDWR = 0x2;
    uint32_t O_CREAT = 0x200;
    uint32_t O_TRUNC = 0x400;
    
    // open file
    int fd = open("truncate_test.txt", O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        printf("failed to open test file");
        return 1;
    }
    
    printf("\nfile size len(fd): %x\n", len(fd));
    
    // fill the file
    char buffer[BUFFER_SIZE];
    my_memset(buffer, 'A', BUFFER_SIZE);
    
    // Test 1: write and verify size
    printf("\nTest 1: write and verify size\n");
    write(fd, buffer, sizeof(buffer));
    int file_size = len(fd);
    print_file(fd, file_size);
    printf("\n");
    
    // Test 2: truncate to half size
    printf("\nTest 2: truncate to half size\n");
    int half_size = BUFFER_SIZE / 2;
    ftruncate(fd, half_size);
    
    // print_result("truncate system call", result == 0);
    file_size = len(fd);
    print_file(fd, file_size);
    printf("\n");


    // Test 3: write at the end of truncated file
    printf("\nTest 3: write at the end of truncated file\n");
    seek(fd, half_size);
    write(fd, TEST_STRING, sizeof(TEST_STRING));
    file_size = len(fd);
    print_file(fd, file_size);
    printf("\n");

    
    // Test 4: truncate to zero
    printf("\nTest 4: truncate to zero\n");
    ftruncate(fd, 0);
    
    // print_result("truncate to zero", result == 0);
    file_size = len(fd);
    print_file(fd, file_size);
    printf("\n");

    
    // Test 5: Write after zero truncate
    printf("\nTest 5: write after zero truncate\n");
    write(fd, TEST_STRING, sizeof(TEST_STRING));
    file_size = len(fd);
    print_file(fd, file_size);
    printf("\n");

    
    // Test 6: truncate to larger size
    printf("\nTest 6: truncate to larger size\n");
    int larger_size = 300;
    ftruncate(fd, larger_size);
    file_size = len(fd);
    printf("file size: %x\n", file_size);
    printf("expected size: %x\n", larger_size);
    print_file(fd, file_size);
    printf("\n");

    close(fd);
    
    printf("\n=== Truncate testing completed ===\n");
    shutdown();
    return 0;
}