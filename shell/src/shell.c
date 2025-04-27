#include "shell.h"
#include "tokenizer.h"

#define TESTING

bool is_numeric(char *str) {
    int index = 0;
    int curr_char = str[index];
    while (curr_char) {
        if (curr_char < '0' || curr_char > '9') {
            return false;
        }
        index++;
        curr_char = str[index];
    }
    return true;
}

void output_prompt() {
    printf("$ ");
}

void make_bin_path(char* buffer, const char* argv0) {
    const char* bin = "/bin/";
    buffer[0] = 0;
    strcat(buffer, bin);
    strcat(buffer, argv0);
}

#ifdef TESTING
// for testing purposes only
// read a line from a file into the destination buffer
// advance the source buffer
char* input_line(char* dst, char* src) {
    int i = 0;
    char c = src[i];
    while (c && c != '\n' && c != '#') {
        dst[i] = c;
        i++;
        c = src[i];
    }
    dst[i] = 0;
    return src[i] ? &src[i + 1] : &src[i];
}
#else
// read keyboard input into buffer
// return on newline
void input_line(char* buffer) {
    uint32_t write = 0;
    char c; // read from keyboard
    read(0, &c, 1);
    while (!is_newline(c)) {
        buffer[write] = c;
        read(0, &c, 1);
        write++;
    }
    buffer[write] = 0;
}
#endif

void handle_built_in(enum CMD_CODES id, Tokenized tokens) {
    switch (id) {
        case CMD_IGNORE: // comment
            printf("\n");
            break;
        case CMD_CD:
            if (tokens.argc != 2) error("invalid args for command cd\n");
            argument_t* argv1 = tokens.argv.next;
            chdir(argv1->val);
            break;
        case CMD_PATH:
            printf("¯\\_(ツ)_/¯\n");
            break;
        case CMD_EXIT:
            exit(0);
            break;
        case CMD_ECHO: {
            argument_t* arg = tokens.argv.next;
            while (arg) {
                printf("%s ", arg->val);
                arg = arg->next;
            }
            printf("\n");
            break;
        }
        case CMD_PWD:
            // todo ask for a syscall
            printf("i think you are in a directory ngl\n");
            break;
        case CMD_LS:
            print_dir();
            break;
        case CMD_SET: {
            argument_t* argv1 = tokens.argv.next;   // key
            argument_t* argv2 = argv1->next;        // value
            int str_len = strlen(argv1->val) + 1;
            char *var_name = malloc(str_len);
            memcpy(var_name, argv1->val, str_len);
            char *var_val = argv2->val;
            value_t p;
            valuemeta_t val_type;
            if (is_numeric(var_val)) {
                val_type = TYPE_INT;
                p.idata = my_atoi(var_val);
            } else {
                val_type = TYPE_STRING;
                p.sdata = malloc(strlen(var_val) + 1);
                memcpy((void *)p.sdata, (void *)var_val, strlen(var_val) + 1);
            }
            hash_put(&environment_variables, var_name, val_type, p);
            break;
        }
        case CMD_GET: {
            argument_t* argv1 = tokens.argv.next;
            argument_t *argv2 = argv1->next;
            HashValue *val = hash_get(&environment_variables, argv2->val);
            if (val->type == TYPE_INT) {
                printf("VAL: %d\n", val->data.idata);
            } else {
                printf("VAL: %s\n", val->data.sdata);
            }
            break;
        }
        // case CMD_PRINT_NUT:
        //     nut();
        //     break;
        // case CMD_GET_NUT_FACT:
        //     nut_fact();
        //     break;
        default:
            error("how did you get here??\n");
    }
}

void handle_fork(const char* path, node_t* node) {
    int id = fork();
    if (id < 0) {
        error("can't fork\n");
    } else if (id == 0) {
        int argc = node->tokens.argc;
        char** argv = malloc(sizeof(void*) * argc);
        argument_t* arg = &node->tokens.argv;
        for (int i = 0; i < argc; i++) { // put in array
            argv[i] = arg->val;
            arg = arg->next;
        }

        shell_exec(path, argv, argc);
        error("couldn't execute command\n");

    } else {
        // parent
        wait(id, &node->exit_status);
    }
}

void process_start(node_t *node) {
    // built in
    enum CMD_CODES command_id = get_command_id(node->tokens.argv.val);
    if (command_id != CMD_EXTERNAL) {
        handle_built_in(command_id, node->tokens);
        node->exit_status = 0;
        return;
    }    

    // elf process ( user executable )
    int fd = open(node->tokens.argv.val, 0);
    if (fd >= 0) {
        uint32_t buffer = 0;
        read(fd, &buffer, sizeof(buffer));
        if (buffer == 0x7f454c46) {
            handle_fork(node->tokens.argv.val, node);
            close(fd);
            return;
        }
    }
    close(fd);

    // elf process ( bin executable )
    const char* bin = "/bin/";
    int length = strlen(node->tokens.argv.val);
    char* path = (char*) malloc(sizeof(bin) + length + 1);
    make_bin_path(path, node->tokens.argv.val);
    fd = open(path, 0);
    if (fd >= 0) {
        handle_fork(path, node);
        free(path);
        close(fd);
        return;
    }
    free(path);
    close(fd);

    // pistash script
    fd = open(node->tokens.argv.val, 0);
    if (fd >= 0) {
        char shebang[15]; 
        read(fd, shebang, 15); // read first 15 bytes of the file (14 + \n)
        shebang[14] = '\0'; 
        if (streq(shebang, "#!/bin/pistash")) { // when we have more interpreters, replace with check for #!
            char *buffer = (void*) malloc(4096);
            read(fd, (void*) buffer, 4096);
            int len_of_file = len(fd) - strlen(shebang) - 1; // for shebang and new line
            if (buffer[len_of_file - 1] == '\n') {
                // file ends in \n
                buffer[len_of_file - 1] = '\0'; 
            } else {
                buffer[len_of_file] = '\0';
            }
            while (*buffer != '\0') {
                buffer = tokenize_and_process(buffer); // start a process from the command
            }
            free(buffer);
        } else {
            printf("couldn't execute file %s\n", node->tokens.argv.val);
        } 
    }
    close(fd);
    // tokens_free(&node->tokens);
}

void inorder_process(node_t* node) {
    if (node) {
        inorder_process(node->left);
        if (node->type == NODE_COMMAND) {
            process_start(node);
            return;
        }

        int rc = node->left->exit_status;
        bool eval_right = (rc == 0 && node->type == NODE_AND) || (rc != 0 && node->type == NODE_OR);
        if (eval_right) {
            inorder_process(node->right);
        }
    }
}

char* tokenize_and_process(char *buffer) {
    char* command = malloc(4096);

    // output_prompt();                             // display a prompt
    buffer = input_line(command, buffer);           // get a line of input, which will become the command to execute
    node_t* ast = tokenize(command);
    inorder_process(ast);

    free(command);
    tokenizer_free(ast);
    return buffer;
}

void welcome() {
    // this font is so funny
    printf("   _ __     _              _                      _         _            \n");
    printf("  | '_ \\   (_)     ___    | |_    __ _     ___   | |_      (_)     ___   \n");
    printf("  | .__/   | |    (_-<    |  _|  / _` |   (_-<   | ' \\     | |    / _ \\  \n");
    printf("  |_|__   _|_|_   /__/_   _\\__|  \\__,_|   /__/_  |_||_|   _|_|_   \\___/  \n");
    printf("_|\"\"\"\"\"|_|\"\"\"\"\"|_|\"\"\"\"\"|_|\"\"\"\"\"|_|\"\"\"\"\"|_|\"\"\"\"\"|_|\"\"\"\"\"|_|\"\"\"\"\"|_|\"\"\"\"\"| \n");
    printf("`'-0-0-''`-0-0-''`-0-0-''`-0-0-''`-0-0-''`-0-0-''`-0-0-''`-0-0-''`-0-0-'  \n");
}

int main(int argc, char** argv) {
    welcome();
    
    char* buffer;
    hash_init(&environment_variables);

    #ifdef TESTING
    int stdin = open("/user/input", 0); // our test case
    buffer = (void*) malloc(4096);
    read(stdin, (void*) buffer, 4096);
    #endif

    while (true) {
        buffer = tokenize_and_process(buffer);
    }

    shutdown(); // should never get here
}
