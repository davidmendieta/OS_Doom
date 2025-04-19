#pragma once
#define MAX_ARG_LEN 100
#include "libc.h"

typedef struct Tokenized {
    char **argv;
    int argc;
} Tokenized;

int increase_index_and_copy_into_temp_array(char *temp_array, char *from_array, int index, int *strlen);

Tokenized *tokenize(char *input);