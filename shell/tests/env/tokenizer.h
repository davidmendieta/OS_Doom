#pragma once

#define MAX_ARG_LEN 100
#include "libc.h"

typedef struct Tokenized {
    char **argv;
    int argc;
} Tokenized;

Tokenized *tokenize(char *input);