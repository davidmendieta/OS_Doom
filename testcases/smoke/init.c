/* Exercise the actual user/kernel boundary without depending on the unfinished libc. */
extern int write(unsigned fd, const void *buf, unsigned size);
extern int open(const char *path, int flags, unsigned mode);
extern int read(unsigned fd, void *buf, unsigned size);
extern int close(unsigned fd);

#define SAY(text) write(1, text, sizeof(text) - 1)

static int equal(const char *a, const char *b, unsigned size) {
    for (unsigned i = 0; i < size; ++i) {
        if (a[i] != b[i]) return 0;
    }
    return 1;
}

int main(int argc, char **argv) {
    SAY("*** smoke: entered user mode\n");
    if (argc != 1 || !argv || !argv[0] || !equal(argv[0], "init", 5) || argv[1]) {
        SAY("*** smoke: FAIL startup arguments\n");
        return 1;
    }

    const char expected[] = "Hello from ext2!\n";
    char data[sizeof(expected)];
    int fd = open("/etc/boot-message", 0, 0);
    if (fd < 0) {
        SAY("*** smoke: FAIL open\n");
        return 1;
    }
    int count = read(fd, data, sizeof(data));
    if (count != sizeof(expected) - 1 || !equal(data, expected, sizeof(expected) - 1)) {
        SAY("*** smoke: FAIL file contents\n");
        return 1;
    }
    if (read(fd, data, 1) != 0 || close(fd) != 0) {
        SAY("*** smoke: FAIL EOF/close\n");
        return 1;
    }
    SAY("*** smoke: PASS user mode, arguments, ext2, syscalls\n");
    return 0;
}
