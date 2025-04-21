#define MAX_ARG_LEN 100
#include "libc.h"
#include <stdbool.h>

typedef struct Tokenized {
    char** argv;
    int argc;
} Tokenized;

// static bool is_newline(char c) {
//     return c == '\n' || c == '\r';
// }

void tokens_init(Tokenized* tokens, const char* input);
void tokens_free(Tokenized* tokens);
void tokenize(Tokenized* tokens, const char* input);