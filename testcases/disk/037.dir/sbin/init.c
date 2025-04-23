#include "libc.h"

int main(int argc, char** argv) {
    printf("*** starting write to indirects testcase\n");

    int fd = open("/pride_and_prejudice", 0, 0);
    char* src = "*** i am writing to the first indirect\n";
    seek(fd, 50000);

    int bytes_written = write(fd, src, 39);
    if (bytes_written == -1) {
        printf("*** write returned - 1\n");
    }

    close(fd);
    int fd1 = open("/pride_and_prejudice", 0, 0);
    char buf[40];
    seek(fd1, 50000);

    int n = read(fd1, buf, 39);
    if (n == -1) {
        printf("*** read returned - 1\n");
    }

    buf[n] = 0;
    printf("%s", buf);

    shutdown();
    return 0;
}


