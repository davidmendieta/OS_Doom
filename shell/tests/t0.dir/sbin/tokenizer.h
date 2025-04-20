#pragma once
#define MAX_ARG_LEN 100
#include "libc.h"

typedef struct Tokenized {
    char **argv;
    int argc;
} Tokenized;

int count_argc(char *input);

char* iterate_and_fill (char* dst, char* src);

Tokenized *tokenize(char *input);