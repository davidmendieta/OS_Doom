#include "libc.h"

int main(int argc, char** argv) {
    printf("*** argc = %d\n",argc);
    for (int i=0; i<argc; i++) {
        printf("*** argv[%d]=%s\n",i,argv[i]);
    }

    
    int fd = open("/data/blank.txt", 0);
    char* arr = naive_mmap(4096, 1, fd, 0);

    for (uint32_t i = 0; i < 4096; i++) {
        arr[i] = '\0';
    }

    int pid = fork();
    if (pid < 0) {
        printf("Fork failed Robotics\n");
        shutdown();
    } else if (pid == 0) {
        for (int i = 0; i < 100; i++) {
            arr[i] = 'a';
        }
        exit(1678);
    } else {
        uint32_t status = 2056;
        wait(pid,&status);
        printf("*** back from wait %ld\n",status);
        for (int i = 0; i < 100; i++) {
            if (arr[i] != 'a') {
                printf("*** Should not print, shared not working\n");
            }
        }
      
    }

    chdir("data");

    printf("Testing mmaping\n");
    int fd1 = open("hello.txt", 0);
    if (fd1 < 0) {
        shutdown();
    }
    char* hello = naive_mmap(4096, 0, fd1, 0);
    printf("%s\n", hello);
    close(fd1);


    return 118;
}
