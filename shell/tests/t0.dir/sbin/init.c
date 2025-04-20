#include "libc.h"
#define STEP1 "1"
#define STEP2 "2"

/**
 * This is a testcase that tests execl and reusing the file with different argv0's
 * heres what happens
 *      kernelMain will launch this file with identity 'init'
 *      init execl's itself with identity 'python3' and passes the argument "1"
 *      python3 execl's itself with identity 'python3' and passes the argument "2" and a string check
 *      python3 prints a terminal message
 */

int streq(const char* s1, const char* s2) {
    while (*s1 && *s2) {
        if (*s1 != *s2) return 0;
        s1++;
        s2++;
    }
    return *s1 == *s2;
}

void error(const char* msg) {
    printf("*** %s\n", msg);
    shutdown();
}

int main(int argc, char** argv) {
    printf("in init\n");
    int id = fork();
    if (id == 0) {
        execl("./sbin/shell", "shell");
    } else if (id > 0) {
        uint32_t status = 0;
        wait(id, &status);
        printf("STATSU: %ld\n", status);
    } else {
        printf("BROEKN");
    }
    shutdown();
}
