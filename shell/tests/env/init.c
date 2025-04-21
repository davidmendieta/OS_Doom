#include "sys.h"
#include "libc.h"

int main(int argc, char** argv) {
    printf("hello from init.c\n");
    int pid = fork();
    if (pid < 0) {
        printf("fork failed\n");
        shutdown();
    } else if (pid == 0) {
        execl("/sbin/shell", "shell", 0);
    } else {
        uint32_t status = 418;
        wait(pid, &status);
        printf("goodbye! (status %ld)\n", status);
        shutdown();
    }
}