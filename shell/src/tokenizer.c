#include "libc.h"
#include "tokenizer.h"

void tokens_init(struct Tokenized* tokens) {
    tokens->argc = 0;
    tokens->argv.val = NULL;
    tokens->argv.next = NULL;
}

void node_init(node_t* node) {
    tokens_init(&node->tokens);
    node->fin = NULL;
    node->fout = NULL;
    node->ferr = NULL;
    node->left = NULL;
    node->right = NULL;
    node->next = NULL;
    node->exit_status = 1;
    node->background_job = false;
}

void node_free(node_t* node) {
    free(node->fin);
    free(node->fout);
    free(node->ferr);

    argument_t* arg = &node->tokens.argv;
    while (arg) {
        argument_t* next = arg->next;
        free(arg->val);
        arg = next;
    }
}

void tokenizer_free(node_t* node) {
    if (node) {
        tokenizer_free(node->left);
        tokenizer_free(node->right);

        if (node->type == NODE_COMMAND) {
            node_free(node);
            free(node);
        }
    }
}

void tokenizer_debug(queue_t* stream) {
    node_t* node = stream->front;
    while (node) {
        switch (node->type) {
            case NODE_COMMAND:
                printf("%s with args:\n", node->tokens.argv.val);
                argument_t* arg = &node->tokens.argv;
                while (arg) {
                    printf("\t%s\n", arg->val);
                    arg = arg->next;
                }
                printf("<end>\n");

                if (node->fin) {
                    printf("--- takes input from %s\n", node->fin);
                }
                if (node->fout) {
                    printf("--- redirects output to %s\n", node->fout);
                }
                if (node->ferr) {
                    printf("--- redirects errors to %s\n", node->ferr);
                }
                if (node->background_job) {
                    printf("--- runs in the background\n");
                }

                break;
            case NODE_AND:
                printf("NODE_AND\n");
                break;
            case NODE_OR:
                printf("NODE_OR\n");
                break;
            case NODE_PIPE:
                printf("NODE_PIPE\n");
                break;
            case NODE_BACKGROUND_JOB:
                printf("NODE_BACKGROUND_JOB\n");
                break;
            case NODE_APPEND:
                printf("NODE_APPEND\n");
                break;
            case NODE_REDIRERR:
                printf("NODE_REDIRERR\n");
                break;
            case NODE_REDIROUT:
                printf("NODE_REDIROUT\n");
                break;
            case NODE_REDIRINP:
                printf("NODE_REDIRINP\n");
                break;
            default:
                printf("unidentifiable node of type %d\n", (int) node->type);
                break;
        }

        node = node->next;
    }
}

int tokenizer_get_node_type(const char* buffer) {
    struct TokenSymbol symbols[] = {
        {"&&", NODE_AND},
        {"||", NODE_OR},
        {">>", NODE_APPEND},
        {"2>", NODE_REDIRERR},
        {"<", NODE_REDIRINP},
        {">", NODE_REDIROUT},
        {"|", NODE_PIPE},
        {"&", NODE_BACKGROUND_JOB},
        {NULL, NODE_COMMAND}
    };
    for (int i = 0; symbols[i].symbol != NULL; i++) {
        if (streq(buffer, symbols[i].symbol)) return symbols[i].type;
    }
    return NODE_COMMAND;
}

void tokenizer_throw_syntax_error(char fault, queue_t* stream) {
    if (fault == '\n') {
        printf("pistash: syntax error near unexpected token `newline\n");
    } else {
        printf("pistash: syntax error near unexpected token %c\n", fault);
    }

    while (peek(stream)) {
        node_t* node = dequeue(stream);
        node_free(node);
        free(node);
    }
}

// push to the end of the argument linked list
// assumes that node.tokens.argv.val has allocated memory
void push_arg(node_t* node, argument_t* arg) {
    argument_t* tail = &node->tokens.argv;
    if (tail->val == NULL) {
        node->tokens.argv = *arg;
        return;
    }
    while (tail->next) {
        tail = tail->next;
    }
    tail->next = arg;
    arg->next = NULL;
}

// convert buffer data into a node_t
// allocates a node_t and char*
node_t* tokenizer_buffer_to_node(char* buffer) {
    if (*buffer == 0) {
        printf("buffer is null %d\n", *(int*)0);
    }

    node_t* node = malloc(sizeof(node_t));
    node_init(node);
    node->type = tokenizer_get_node_type(buffer);
    if (node->type == NODE_COMMAND) {
        int length = strlen(buffer);
        node->tokens.argv.val = malloc(length + 1);
        memcpy(node->tokens.argv.val, buffer, length);
        node->tokens.argv.val[length] = 0;
        push_arg(node, &node->tokens.argv);
    }

    return node;
}

// flush the buffer, set bi to 0
// does not allocate any memory
node_t* tokenizer_buffer_flush(char* buffer, int* bi_) {
    int bi = *bi_;
    buffer[bi] = 0;
    *bi_ = 0;
    node_t* node = tokenizer_buffer_to_node(buffer);
    return node;
}

// process the src stream
// create well formed commands separated by operators
void tokenizer_assign_args(queue_t* stream, queue_t* src) {
    node_t* prev = NULL;
    while (peek(src)) {
        node_t* node = dequeue(src);
        switch (node->type) {
            case NODE_BACKGROUND_JOB:
                prev->background_job = true;
                break;
            case NODE_REDIRINP:
                // prev->fin = node->tokens.argv.val;
                prev->fin = dequeue(src)->tokens.argv.val;
                break;
            case NODE_REDIROUT:
                // prev->fout = node->tokens.argv.val;
                prev->fout = dequeue(src)->tokens.argv.val;
                break;
            case NODE_REDIRERR:
                // prev->ferr = node->tokens.argv.val;
                prev->ferr = dequeue(src)->tokens.argv.val;
                break;
            case NODE_APPEND:
                // todo handle
                break;
            case NODE_AND:
            case NODE_OR:
            case NODE_PIPE:
                prev = node;
                enqueue(stream, node);
                break;
            case NODE_COMMAND:
                if (prev && prev->type == NODE_COMMAND) { // arg
                    push_arg(prev, &node->tokens.argv);
                    prev->tokens.argc += 1;
                } else {
                    prev = node;
                    enqueue(stream, node);
                    node->tokens.argc = 1;
                }
                break;
            default:
                break;
        }
    }
}

// convert user input into a tokenstream (queue)
// allocates a queue_t
queue_t* tokenizer_tokenstream(const char* input) {
    /**
     * todo handle \" as variable expansions 
     * todo handle & at end of expressions
     */

    queue_t stream_;
    init_queue(&stream_);
    const char* p = input;
    char* buffer = malloc(1024);
    int bi = 0;
    bool in_single = false, in_double = false;
    bool require_next = false;

    while (*p) {
        char c = *p;
        if (!in_single && !in_double && c == ' ') { // command or argument
            if (bi > 0) {
                node_t* node = tokenizer_buffer_flush(buffer, &bi);
                enqueue(&stream_, node);
                require_next = false;
            }
            p++;
            continue;
        }

        if (!in_single && !in_double && c == '#') { // comments
            break;
        }

        if (c == '\'' && !in_double) {
            in_single = !in_single;
            p++;
            continue;
        }

        if (c == '\"' && !in_single) {
            in_double = !in_double;
            p++;
            continue;
        }

        if (!in_single && c == '\\') { // escape sequence
            p++;
            if (*p) buffer[bi++] = *p++;
            continue;
        }

        if (!in_single && !in_double && strncmp(p, "2>", 2) == 0) { // 2> specifically
            if (require_next) {
                tokenizer_throw_syntax_error(c, &stream_);
                return NULL;
            }

            // flush the buffer
            if (bi > 0) {
                node_t* node = tokenizer_buffer_flush(buffer, &bi);
                enqueue(&stream_, node);
            }

            buffer[bi++] = '2';
            buffer[bi++] = '>';
            node_t* node = tokenizer_buffer_flush(buffer, &bi);
            enqueue(&stream_, node);
            p += 2;
            require_next = true;
            continue;
        }

        if (!in_single && !in_double && strchr("|&>", c)) {
            /**
             * the current token might be
             *      && - logical and
             *      || - logical or
             *      >> - append
             *      &  - run in background
             *      |  - pipe
             *      >  - redirect output
             */

            if (require_next) {
                tokenizer_throw_syntax_error(c, &stream_);
                return NULL;
            }

            // flush the buffer
            if (bi > 0) {
                node_t* node = tokenizer_buffer_flush(buffer, &bi);
                enqueue(&stream_, node);
            }

            // check double operators
            if (*(p+1) && c == *(p+1)) {
                buffer[bi++] = c;
                buffer[bi++] = c;
                p++;
            } else {
                buffer[bi++] = c;
            }
            node_t* node = tokenizer_buffer_flush(buffer, &bi);
            enqueue(&stream_, node);
            p++;
            require_next = true;
            continue;
        }

        // normal character
        buffer[bi++] = c;
        p++;
    }
    if (bi > 0) {
        node_t* node = tokenizer_buffer_flush(buffer, &bi);
        enqueue(&stream_, node);
    }
    if (require_next && stream_.rear->type != NODE_COMMAND) {
        tokenizer_throw_syntax_error('\n', &stream_);
        return NULL;
    }

    free(buffer);

    queue_t* stream = malloc(sizeof(queue_t));
    init_queue(stream);

    tokenizer_assign_args(stream, &stream_);    
    // tokenizer_debug(stream);
    return stream;
}

void tokenizer_form_tree(queue_t* stream, enum NodeType op) {
    bool work = true;

    while (work) {
        work = false;
        queue_t next_q;
        init_queue(&next_q);

        while (peek(stream)) {
            node_t* left = dequeue(stream);
            node_t* operator = peek(stream);
            if (operator && operator->type == op) {
                operator = dequeue(stream);
                if (peek(stream)) {
                    work = true;
                    node_t* right = dequeue(stream);
                    operator->left = left;
                    operator->right = right;
                    enqueue(&next_q, operator);
                } else {
                    enqueue(&next_q, left);
                    enqueue(&next_q, operator);
                }
            } else { // not this operator's turn or no operator at all
                enqueue(&next_q, left);
            }
        }

        stream->front = next_q.front;
        stream->rear = next_q.rear;
    
    }
}

node_t* tokenizer_ast(queue_t* stream) {
    tokenizer_form_tree(stream, NODE_AND);
    tokenizer_form_tree(stream, NODE_OR);
    tokenizer_form_tree(stream, NODE_PIPE);
    return peek(stream);
}

// return the root of the ast representing user input
// return NULL on error
node_t* tokenize(const char* input) {
    if (*input == 0) return NULL;

    queue_t* stream = tokenizer_tokenstream(input);
    if (stream == NULL) {
        return NULL;
    }
    node_t* ast = tokenizer_ast(stream);
    free_queue(stream);
    return ast;
}