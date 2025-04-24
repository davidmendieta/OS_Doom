#include "libc.h"
#include "tokenizer.h"

bool is_newline(char c) {
    return c == '\n' || c == '\r';
}

bool is_path(char c) {
    return c == '.' || c == '/';
}

int count_argc(const char *input) {
    int ret = 0;
    int index = 0;
    while (input[index] != '\0') {
        if (input[index] == ' ') {
            if (!input[index + 1]) {
                // space before null terminator
                return ret + 1;
            }
            ret++;
        }
        index++;
    }
    return ret + 1;
}

const char* iterate_and_fill (char* dst, const char* src) {
    int i = 0;
    char c = *(src + i);
    while (c && c != ' ') {
        dst[i] = c;
        i++;
        c = *(src + i);
    }
    dst[i] = 0;
    return (src + i + 1);
}

void tokens_init(Tokenized* tokens, const char* input) {
    // empty line, return
    tokens->argv = 0;
    if (input[0] == 0) return;

    int argc = count_argc(input);
    tokens->argc = argc;
    tokens->argv = (char**) malloc(sizeof(char*) * (argc + 1));
    tokens->argv[argc] = 0;
}

void tokens_free(Tokenized* tokens) {
    for (int i = 0; i < tokens->argc; i++) {
        free(tokens->argv[i]);
    }
    free(tokens->argv);
}

void tokenize(Tokenized* tokens, const char *input) {
    // fill argv with each token
    // "ls -al" --> ["ls", "-al", 0]
    char* temp = malloc(4096);
    for (int i = 0; i < tokens->argc; i++) {
        input = iterate_and_fill(temp, input);
        int len = strlen(temp);
        tokens->argv[i] = malloc(len + 1);
        memcpy((void *)tokens->argv[i], (void *)temp, len + 1);
        if (tokens->argv[i][0] == '&') {
            tokens->background_process = true;
        }
        // printf("ARG %d: %s\n", i, tokens->argv[i]);
    }
    free(temp);
}

bool is_operator(const char* s, int i) {
    return  s[i] == '|' || 
            (s[i] == '&' && s[i+1] && s[i+1] == '&') ||
            (s[i] == '|' && s[i+1] && s[i+1] == '|');
}

enum NodeType get_operator(const char* s, int i) {
    
    if (s[i] == '&' && s[i+1] && s[i+1] == '&') {
        printf("Should set type correctly\n");
        return NODE_AND;
    } else if (s[i] == '|' && s[i+1] && s[i+1] == '|') {
        return NODE_OR;
    } else {
        return NODE_PIPE;
    }
}

node_t* buffer_to_node(const char* buffer) {
    node_t* out = malloc(sizeof(node_t));
    // memset(out, 0, sizeof(node_t));
    char* temp = malloc(4096);
    int argc = count_argc(buffer); // todo give up when i read redirection
    out->tokens.argc = argc;
    out->tokens.argv = malloc(argc * (sizeof(char *) + 1));
    out->tokens.argv[argc] = 0;

    // buffer -> echo cat
    // 1 temp -> echo
    // 2 temp -> cat
    // printf("\n________\nbuffer: %s\n", buffer);
    // printf("arc: %d\n", argc);

    for (int i = 0; i < argc; i++) {
        buffer = iterate_and_fill(temp, buffer);
        int len = strlen(temp);
        // printf("Temp arg: %s\n", temp);
        // printf("Past memcpy\n");
        out->tokens.argv[i] = malloc(len + 1);
        memcpy((void *)out->tokens.argv[i], (void *)temp, len + 1);
        if (out->tokens.argv[i][0] == '&') {
            out->background_job = true;
        }
        // printf("Copied into node\n\n");
    }
    // printf("out of loop\n");

    // buffer may contain redirection blocks here
    while (*buffer) {
        if (*buffer == ' ') {
            buffer++;
            continue;
        }
        if (*buffer == '>') {
            out->fout = malloc(100);
            // memset(out->fout, 0, 100);
            buffer = iterate_and_fill(out->fout, buffer+2); // assumes >_file.txt
        } else if (*(buffer+1) && *buffer == '2' && *(buffer+1) == '>') {
            // check 2>)
            out->ferr = malloc(100);
            // memset(out->ferr, 0, 100);
            buffer = iterate_and_fill(out->ferr, buffer+3); // assumes 2>_file.txt
        } else {
            buffer++;
        }
    }

    
    free(temp);
    
    return out;
}

queue_t* input_to_list(char* s) {
    char* buf = malloc(4096);
    int write = 0;
    queue_t *q = malloc(sizeof(queue_t));
    init_queue(q);

    for (int i = 0; s[i]; i++) {
        if (is_operator(s, i)) {
            int num_to_subtract = 0;
            if (s[i + 1] == '&' || s[i + 1] == '|') {
                s += 2;
                num_to_subtract = 2;
            } else {
                s += 1;
                num_to_subtract = 1;
            }
            // process !!!
            // buffer has something like `echo hi > out.txt`
            buf[write] = 0;
            node_t* new_node = buffer_to_node(buf);
            new_node->type = NODE_COMMAND;
            enqueue(q, new_node); // left cmd

            // make the operator node
            new_node = malloc(sizeof(node_t));
            new_node->type = get_operator(s - num_to_subtract, i);
            enqueue(q, new_node); // op
            write = 0;
        } else {
            buf[write] = s[i];
            write++;
        }
    }
    buf[write] = 0;
    node_t* new_node = buffer_to_node(buf);
    new_node->type = NODE_COMMAND;
    enqueue(q, new_node); 
    return q;
}

void form_tree(queue_t* curr_q, enum NodeType op) {
    bool work_done = true;

    while (work_done) {
        work_done = false;
        queue_t next_q;
        init_queue(&next_q);

        while (peek(curr_q)) {
            node_t* left = dequeue(curr_q);
            node_t* operator = peek(curr_q);
            if (operator && operator->type == op) {
                operator = dequeue(curr_q); // guaranteed to exist
                if (peek(curr_q)) {
                    work_done = true;
                    node_t* right = dequeue(curr_q);
                    operator->left = left;
                    operator->right = right;
                    enqueue(&next_q, operator);
                } else {
                    enqueue(&next_q, left);
                    enqueue(&next_q, operator);
                }
            } else {
                enqueue(&next_q, left);
            }
        }

        curr_q->front = next_q.front;
        curr_q->rear = next_q.rear;
    }
}

node_t* list_to_tree(queue_t* list) {
    form_tree(list, NODE_AND);
    form_tree(list, NODE_OR);
    form_tree(list, NODE_PIPE);
    return peek(list);
}