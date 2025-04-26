#pragma once
#include "libc.h"

enum NodeType {
    NODE_AND,
    NODE_OR,
    NODE_PIPE,
    NODE_APPEND,
    NODE_REDIRINP,
    NODE_REDIROUT,
    NODE_REDIRERR,
    NODE_BACKGROUND_JOB,
    NODE_COMMAND
};

struct TokenSymbol {
    const char* symbol;
    enum NodeType type;
};

typedef struct Argument {
    char* val;
    struct Argument* next;
} argument_t;

typedef struct Tokenized {
    argument_t argv;
    int argc;
} Tokenized;

typedef struct Node {
    Tokenized tokens;
    char* fin;
    char* fout;
    char* ferr;
    struct Node* left;
    struct Node* right;
    struct Node* next; // for building
    uint32_t exit_status;
    enum NodeType type;
    bool background_job;
} node_t;
