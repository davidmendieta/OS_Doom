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
    printf("[debugging] argc %d\n",argc);
    for (int i=0; i<argc; i++) {
        printf("[debugging] argv %s\n",argv[i]);
    }

    const char* me = argv[0];

    if (streq(me, "init")) {
        // launches itself as `python3` pass argument 1
        execl("/sbin/init", "python3", STEP1, 0);
    } else if (streq(me, "python3")) {

        const char* step = argv[1];
        if (streq(step, STEP1)) {
            // reads some data from the file
            // check if it is as expected
            // launches itself as `python3` pass argument 2 with the string "good" or "bad"
            // expects to read the string "__main__"
            int fd = open("/my_first_program/main.py", 0);
            char buf[9];

            seek(fd, 55);
            read(fd, (void*)buf, 8);
            buf[8] = 0; 
            
            int py__main__ = streq(buf, "__main__");
            const char* argv1 = py__main__ == 1 ? "good" : "bad";
            execl("/sbin/init", "python3", STEP2, argv1, 0);

        } else if (streq(step, STEP2)) {
            // reads some more data from the file
            // check if the result from step 1 was good
            // expects to read the string "hello world!"

            const char* success = argv[2];
            int fd = open("/my_first_program/main.py", 0);
            char buf[100];

            if (streq(success, "good")) {
                seek(fd, 23);
                read(fd, (void*) buf, 12);
                buf[12] = 0;
                printf("*** %s\n", buf);
                shutdown();
            } else {
                // funny
                seek(fd, 83);
                read(fd, (void*) buf, 32);
                buf[32] = 0;
                printf("%s\n", buf);
                error("python3 2 is right but something went wrong during file reading");
            }

        } else {
            error("python3 is right something else went wrong");
        }

    } else {
        error("cant parse argv0");
    }

    error("execl returned");
    return 0;
}
