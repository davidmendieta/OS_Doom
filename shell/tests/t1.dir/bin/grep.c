#include "libc.h"

char* get_line(char* data, char* buffer) {
    int i = 0;
    while (data[i] && data[i] != '\n') {
        buffer[i] = data[i];
        i++;
    }
    buffer[i] = 0;
    return data[i] ? &data[i + 1] : &data[i];
}

int main(int argc, char** argv) {
    if (!argv[1]) {
        printf("grep: cannot detect empty pattern\n");
        return -1;
    }
    // assume stdin is set up for me
    int length = len(0);
    // int fd = open(argv[2], 0);
    // int length = len(fd);
    int pattern_length = strlen(argv[1]);
    char* text = malloc(length+1);
    char* line = malloc(length+1);

    read(0, text, length);
    // read(fd, text, length);
    text[length] = 0;

    int found = -1;

    while (*text) {
        text = get_line(text, line);
        int line_length = strlen(line);

        // find matches within the line
        // i use a sliding window here
        for (int i = 0; i <= line_length - pattern_length; i++) {
            if (strncmp(argv[1], line, pattern_length) == 0) {
                found = 0;
                printf("%s\n", line);
                break;
            }
        }
    }

    return found;
}
