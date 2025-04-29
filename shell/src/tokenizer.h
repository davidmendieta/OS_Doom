#pragma once
#include "libc.h"
#include "queue.h"
#include "hashmap.h"

node_t* tokenize(const char*);
void tokenizer_free(node_t*);
void tokenize_debug(const char*);
extern struct Hashmap environment_variables;