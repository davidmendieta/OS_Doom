#include "libc.h"
#include "tokenizer.h"

bool is_newline(char c) {
    return c == '\n' || c == '\r';
}

bool is_path(char c) {
    return c == '.' || c == '/';
}

int count_argc(const char *input) {
    int ret = 0;
    int index = 0;
    while (input[index] != '\0') {
        if (input[index] == ' ') {
            ret++;
        }
        index++;
    }
    return ret + 1;
}

const char* iterate_and_fill (char* dst, const char* src) {
    int i = 0;
    char c = *(src + i);
    while (c && c != ' ') {
        dst[i] = c;
        i++;
        c = *(src + i);
    }
    dst[i] = 0;
    return (src + i + 1);
}

void tokens_init(Tokenized* tokens, const char* input) {
    // empty line, return
    tokens->argv = 0;
    if (input[0] == 0) return;

    int argc = count_argc(input);
    tokens->argc = argc;
    tokens->argv = (char**) malloc(sizeof(char*) * (argc + 1));
    tokens->argv[argc] = 0;
}

void tokens_free(Tokenized* tokens) {
    for (int i = 0; i < tokens->argc; i++) {
        free(tokens->argv[i]);
    }
    free(tokens->argv);
}

void tokenize(Tokenized* tokens, const char *input) {
    // fill argv with each token
    // "ls -al" --> ["ls", "-al", 0]
    char* temp = malloc(4096);
    for (int i = 0; i < tokens->argc; i++) {
        input = iterate_and_fill(temp, input);
        int len = strlen(temp);
        tokens->argv[i] = malloc(len + 1);
        memcpy((void *)tokens->argv[i], (void *)temp, len + 1);
        
        // printf("ARG %d: %s\n", i, tokens->argv[i]);
    }
    free(temp);
}