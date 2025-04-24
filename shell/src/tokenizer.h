#define MAX_ARG_LEN 100
#include "libc.h"
#include <stdbool.h>

typedef struct Tokenized {
    char** argv;
    int argc;
    bool background_process;
} Tokenized;

bool is_newline(char c);
bool is_path(char c);

void tokens_init(Tokenized* tokens, const char* input);
void tokens_free(Tokenized* tokens);
void tokenize(Tokenized* tokens, const char* input);