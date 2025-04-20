#include "libc.h"
#include "tokenizer.h"

int count_argc(char *input) {
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

char* iterate_and_fill (char* dst, char* src) {
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

Tokenized *tokenize(char *input)
{
    if (input[0] == 0)
    {
        // empty line, return
        return 0;
    }
    Tokenized *token = (Tokenized *) malloc(sizeof(Tokenized));
    token->argc = count_argc(input);
    printf("ARC in tokenize: %d\n", token->argc);
    token->argv = (char **) malloc(sizeof(char *) * (token->argc + 1));
    token->argv[token->argc] = 0;

    int index = 0;
    while (index < token->argc) {
        char *temp_buffer = malloc(4096);
        input = iterate_and_fill(temp_buffer, input);
        int len = strlen(temp_buffer);
        token->argv[index] = malloc(len + 1);
        memcpy((void *)token->argv[index], (void *)temp_buffer, len + 1);
        free(temp_buffer);
        index++;
    }
    for (int i = 0; i < token->argc; i++) {
        printf("ARG %d: %s\n", i, token->argv[i]);
    }
    return token;
}