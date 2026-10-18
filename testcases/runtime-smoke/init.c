#include <assert.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <os/runtime.h>

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    puts("*** runtime: starting");
    char *p = calloc(8193, 1);
    assert(p && p[8192] == 0);
    memset(p, 0x5a, 8193);
    p = realloc(p, 2 * 1024 * 1024);
    assert(p && p[8192] == 0x5a);
    p[2 * 1024 * 1024 - 1] = 42;
    free(p);
    assert(!fopen("/missing-file", "r"));
    FILE *f = fopen("/etc/boot-message", "rb");
    assert(f);
    struct stat st;
    assert(!fstat(fileno(f), &st) && S_ISREG(st.st_mode) && st.st_size == 17);
    char text[64] = {0};
    assert(fread(text, 1, 5, f) == 5 && !memcmp(text, "Hello", 5));
    assert(!fseek(f, -2, SEEK_END) && ftell(f) == 15);
    assert(fgetc(f) == '!' && fgetc(f) == '\n' && fgetc(f) == EOF);
    assert(!fseek(f, 0, SEEK_SET) && !fseek(f, 6, SEEK_CUR));
    assert(fgetc(f) == 'f');
    assert(!fclose(f));
    /* Cross the sector-cache capacity and ext2's direct/indirect boundary. */
    f = fopen("/etc/runtime-data", "w+b");
    assert(f);
    unsigned char block[4096];
    for (int i = 0; i < 160; i++) {
        memset(block, i, sizeof(block));
        assert(fwrite(block, 1, sizeof(block), f) == sizeof(block));
    }
    assert(!fclose(f));
    f = fopen("/etc/runtime-data", "rb");
    assert(f);
    assert(!fstat(fileno(f), &st) && st.st_size == 160 * 4096);
    for (int i = 159; i >= 0; i--) {
        assert(!fseek(f, i * 4096, SEEK_SET));
        assert(fread(block, 1, sizeof(block), f) == sizeof(block));
        for (unsigned j = 0; j < sizeof(block); j++) assert(block[j] == i);
    }
    assert(!fclose(f));
    uint32_t start = os_milliseconds();
    /* Keep a live x87 value across scheduling/migration, then exercise libm. */
    double result;
    __asm__ volatile("fld1");
    while ((uint32_t)(os_milliseconds() - start) < 100) os_yield();
    __asm__ volatile("fstpl %0" : "=m"(result));
    assert(result == 1.0);
    volatile double nine = 9.0;
    assert(fabs(sqrt(nine) - 3.0) < 0.000001);
    snprintf(text, sizeof(text), "%.2f %d", result, 42);
    assert(!strcmp(text, "1.00 42"));
    puts("*** runtime: PASS allocation, files, seeking, clock, x87, libc");
    return 0;
}
