#pragma once

#define MAX_ARG_LEN 100
#include "libc.h"

typedef struct Tokenized {
    char** argv;
    int argc;
} Tokenized;

void tokens_init(Tokenized* tokens, const char* input);
void tokens_free(Tokenized* tokens);
void tokenize(Tokenized* tokens, const char* input);