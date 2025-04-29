#include "libc.h"

int main(int argc, char** argv) {
    int fd = open(argv[1], 0);
    int length = len(fd);
    char* buffer = (char*) malloc(length + 1);
    read(fd, buffer, length);
    buffer[length] = 0;
    printf("%s\n", buffer);
    exit(0);
}
