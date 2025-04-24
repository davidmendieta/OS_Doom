#pragma once

#define MAX_ARG_LEN 100
#include "libc.h"
#include <stdbool.h>
#include "queue.h"

typedef struct Hashmap Hashmap;

bool is_newline(char c);
bool is_path(char c);

void tokens_init(Tokenized* tokens, const char* input);
void tokens_free(Tokenized* tokens);
void tokenize(Tokenized* tokens, const char* input);

queue_t* input_to_list(char* s, Hashmap* environment_variables);
node_t* list_to_tree(queue_t* list);
