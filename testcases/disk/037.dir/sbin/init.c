#include "libc.h"

int main(int argc, char** argv) {
    printf("starting write to indirects testcase\n");
    int fd = open("/sbin/fortunes", 0, 0);
    // char* src = "*** i am writing to an indirect\n";
    // seek(fd, 50000);
    char buf[50];
    int n = read(fd, buf, 20);
    buf[n] = 0;
    printf("%s", buf);

    shutdown();
    return 0;
}


