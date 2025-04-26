#pragma once
#include "libc.h"
#include "queue.h"

node_t* tokenize(const char*);
void tokenizer_free(node_t*);