#include "sys.h"
#include "libc.h"
#include "uio.h" // new code

void test_writev() {
    // open a file for writing
    const char* filename = "testfile.txt";
    uint32_t fd = open(filename, 0);
    if (fd < 0) {
        printf("failed to open file: %s\n", filename);
        return;
    }

    // set up iovec buffers
    const char* str1 = "Hello, ";
    const char* str2 = "world!";
    const char* str3 = " This is a test of writev.\n";

    struct iovec iov[3];
    iov[0].iov_base = (void*)str1;
    iov[0].iov_len = 7;
    iov[1].iov_base = (void*)str2;
    iov[1].iov_len = 6;
    iov[2].iov_base = (void*)str3;
    iov[2].iov_len = 30;

    printf("iov[0] base: %p, len: %d\n", iov[0].iov_base, iov[0].iov_len);
    printf("iov[1] base: %p, len: %d\n", iov[1].iov_base, iov[1].iov_len);
    printf("iov[2] base: %p, len: %d\n", iov[2].iov_base, iov[2].iov_len);

    // call writev
    uint32_t total_written = writev(fd, (void*)iov, 3);
    if (total_written < 0) {
        printf("writev failed with error: %ld\n", total_written);
        close(fd);
        return;
    }

    // results
    printf("Total bytes written: %ld\n", total_written);
    seek(fd, 0);
    char result[42];
    int n = read(fd, result, 43);
    result[n] = 0;
    printf("*** %s\n", result);

    // don't forget to close the file!
    close(fd);
}


int main(int argc, char **argv) {
  
  test_writev();
  
  shutdown();

  return 0;
}
