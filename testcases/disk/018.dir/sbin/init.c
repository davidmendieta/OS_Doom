
#include "sys.h"
#include "libc.h"
#include <stddef.h> // for size_t

#define BUFFER_SIZE 8  
#define TEST_STRING "The quick brown fox jumps over the lazy dog."

void print_result(const char* test_name, int result) {
    if (result) {
        printf("PASS: %s\n", test_name);
    } else {
        printf("FAIL: %s\n", test_name);
    }
}

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
    
    printf("file size len(fd): %x\n", len(fd));
    
    // fill the file
    char buffer[BUFFER_SIZE];
    my_memset(buffer, 'A', BUFFER_SIZE);
    
    // Test 1: write and verify size
    printf("\nTest 1: write and verify size\n");
    int bytes_written = write(fd, buffer, sizeof(buffer));
    int file_size = len(fd);
    
    print_result("write system call", bytes_written == sizeof(buffer));
    print_result("file size verification", file_size == sizeof(buffer));
    printf("file size: %x\n", file_size);
    printf("expected size: %x\n", BUFFER_SIZE);
    printf("sizeof(buffer): %x\n", sizeof(buffer));
    print_file(fd, file_size);
    printf("\n");
    
    // Test 2: truncate to half size
    printf("\nTest 2: truncate to half size\n");
    int half_size = BUFFER_SIZE / 2;
    int result = ftruncate(fd, half_size);
    
    print_result("truncate system call", result == 0);
    file_size = len(fd);
    print_result("file size after truncate", file_size == half_size);
    printf("file size: %x\n", file_size);
    printf("expected size: %x\n", half_size);
    print_file(fd, file_size);
    printf("\n");


    // Test 3: write at the end of truncated file
    printf("\nTest 3: write at the end of truncated file\n");
    seek(fd, half_size);
    bytes_written = write(fd, TEST_STRING, sizeof(TEST_STRING));
    
    print_result("write after truncate", bytes_written == sizeof(TEST_STRING));
    file_size = len(fd);
    print_result("file size after write", file_size == (half_size + sizeof(TEST_STRING)));
    printf("file size: %x\n", file_size);
    printf("expected size: %x\n",  (half_size + sizeof(TEST_STRING)));
    print_file(fd, file_size);
    printf("\n");

    
    // Test 4: truncate to zero
    printf("\nTest 4: truncate to zero\n");
    result = ftruncate(fd, 0);
    
    print_result("truncate to zero", result == 0);
    file_size = len(fd);
    print_result("file size after truncate to zero", file_size == 0);
    printf("file size: %x\n", file_size);
    print_file(fd, file_size);
    printf("\n");

    
    // Test 5: Write after zero truncate
    printf("\nTest 5: write after zero truncate\n");
    bytes_written = write(fd, TEST_STRING, sizeof(TEST_STRING));
    
    print_result("write after zero truncate", bytes_written == sizeof(TEST_STRING));
    file_size = len(fd);
    print_result("file size verification", file_size == sizeof(TEST_STRING));
    printf("file size: %x\n", file_size);
    printf("expected size: %x\n", sizeof(TEST_STRING));
    print_file(fd, file_size);
    printf("\n");

    
    // Test 6: truncate to larger size
    printf("\nTest 6: truncate to larger size\n");
    int larger_size = BUFFER_SIZE * 2;
    result = ftruncate(fd, larger_size);
    
    print_result("truncate to larger size", result == 0);
    file_size = len(fd);
    print_result("file size after extension", file_size == larger_size);
    printf("file size: %x\n", file_size);
    printf("expected size: %x\n", larger_size);
    print_file(fd, file_size);
    printf("\n");

    
    // Test 7: test with direct + indirect blocks
    printf("\nTest 7: test with direct + indirect blocks\n");
    // truncate to zero
    ftruncate(fd, 0);
    
    // write data that will span multiple blocks
    int block_spanning_size = 16 * 4096; // will span 16 blocks
    char* large_buffer = malloc(block_spanning_size);
    if (!large_buffer) {
        printf("failed to allocate memory");
        close(fd);
        return 1;
    }
    
    // fill the file
    for (int i = 0; i < block_spanning_size; i++) {
        large_buffer[i] = 'A' + (i % 26);
    }
    
    // write the buffer to the file
    bytes_written = write(fd, large_buffer, block_spanning_size);
    print_result("Write large data", bytes_written == block_spanning_size);
    
    // truncate to a size that cuts off some blocks
    int truncated_size = 10 * 4096; // keep only 10 blocks
    result = ftruncate(fd, truncated_size);
    print_result("Truncate multi-block file", result == 0);
    
    file_size = len(fd);
    print_result("File size after multi-block truncate", file_size == truncated_size);
    printf("file size: %x\n", file_size);
    printf("expected size: %x\n", truncated_size);
    print_file(fd, file_size);
    printf("\n");

    
    // clean up
    free(large_buffer);
    close(fd);
    
    printf("\n=== Truncate testing completed ===\n");
    shutdown();
    return 0;
}