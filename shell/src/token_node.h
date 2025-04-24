#pragma once

enum NodeType {
    NODE_AND,
    NODE_OR,
    NODE_PIPE,
    NODE_COMMAND
};

typedef struct Tokenized {
    char** argv;
    int argc;
    bool background_process;
} Tokenized;

typedef struct Node {
    Tokenized tokens;
    char* fin;
    char* fout;
    char* ferr;
    struct Node* left;
    struct Node* right;
    struct Node* next; // for building
    int exit_status;
    enum NodeType type;
    bool background_job;
} node_t;