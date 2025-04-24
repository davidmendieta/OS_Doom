#include "shell.h"
#include "tokenizer.h"
#include "hashmap.h"

Hashmap environment_variables;

#define TESTING

void output_prompt() {
    printf("$ ");
}

bool check_variable_is_int(char *str) {
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

#ifdef TESTING
// for testing purposes only
// read a line from a file into the destination buffer
// advance the source buffer
char* input_line(char* dst, char* src) {
    int i = 0;
    char c = *(src + i);
    while (c && c != '\n') {
        dst[i] = c;
        i++;
        c = *(src + i);
    }
    dst[i] = 0;
    if (c == '\0') return (src + i);
    return (src + i + 1);
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
            chdir(tokens.argv[1]);
            break;
        case CMD_PATH:
            printf("¯\\_(ツ)_/¯\n");
            break;
        case CMD_EXIT:
            exit(0);
            break;
        case CMD_ECHO:
            // todo we need a more powerful tokenizer :_(
            for (int i = 1; i < tokens.argc; i++) {
                printf("%s ", tokens.argv[i]);
            }
            printf("\n");
            break;
        case CMD_PWD:
            // todo ask for a syscall
            printf("i think you are in a directory ngl\n");
            break;
        case CMD_LS:
            print_dir();
            break;
        case CMD_SET: {
            int str_len = strlen(tokens.argv[1]) + 1;
            char *var_name = malloc(str_len);
            memcpy((void *)var_name, (void *) tokens.argv[1], str_len);
            char *var_val = tokens.argv[2];
            value_t p;
            valuemeta_t val_type;
            if (check_variable_is_int(var_val)) {
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
            char *var_name = tokens.argv[1];
            HashValue *val = hash_get(&environment_variables, var_name);
            if (val->type == TYPE_INT) {
                printf("VAL: %ld\n", val->data.idata);
            } else {
                printf("VAL: %s\n", val->data.sdata);
            }
            break;
        }
        default:
            error("how did you get here??\n");
    }
}

void handle_fork(const char* path, Tokenized* tokens) {
    int id = fork();
    if (id < 0) {
        error("can't fork\n");
    } else if (id == 0) {
        shell_exec(path, tokens->argv, tokens->argc);
        error("couldn't execute command\n");
    } else {
        // parent
        if (!tokens->background_process) {
            uint32_t status = 42;
            wait(id, &status);
            printf("process exited with status %ld\n", status);
        }
    }
}

void make_bin_path(char* buffer, const char* argv0) {
    const char* bin = "/bin/";
    buffer[0] = 0;
    strcat(buffer, bin);
    strcat(buffer, argv0);
}

void post_order_process(node_t* node);

char * tokenize_and_process(char *buffer) {
    char* command = malloc(4096);
    output_prompt();                             // display a prompt
    buffer = input_line(command, buffer);        // get a line of input, which will become the command to execute
    node_t* ast = list_to_tree(input_to_list(command));
    post_order_process(ast);
    // process_start(command);                      // start a process from the command
    return buffer;
}

void process_start(node_t *node) {

    // built in
    enum CMD_CODES command_id = get_command_id(node->tokens.argv);
    if (command_id != CMD_EXTERNAL) {
        handle_built_in(command_id, node->tokens);
        tokens_free(&node->tokens);
        return;
    }    

    // elf process ( user executable )
    int fd = open(node->tokens.argv[0], 0);
    uint32_t buffer = 0;
    read(fd, &buffer, sizeof(buffer));
    if (buffer == 0x7f454c46) {
        handle_fork(node->tokens.argv[0], &node->tokens);
        tokens_free(&node->tokens);
        close(fd);
        return;
    }
    close(fd);

    // elf process ( bin executable )
    const char* bin = "/bin/";
    int length = strlen(node->tokens.argv[0]);
    char* path = (char*) malloc(sizeof(bin) + length + 1);
    make_bin_path(path, node->tokens.argv[0]);
    fd = open(path, 0);
    if (fd >= 0) {
        handle_fork(path, &node->tokens);
        tokens_free(&node->tokens);
        free(path);
        close(fd);
        return;
    }
    free(path);
    close(fd);

    // pistash script
    fd = open(node->tokens.argv[0], 0);
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
            printf("couldn't execute file %s\n", node->tokens.argv[0]);
        } 
    }
    close(fd);
    // tokens_free(&node->tokens);
}

void welcome() {
    // this font is so funny
    printf("   _ __     _              _                      _         _            \n");
    printf("  | '_ \\   (_)     ___    | |_    __ _     ___   | |_      (_)     ___   \n");
    printf("  | .__/   | |    (_-<    |  _|  / _` |   (_-<   | ' \\     | |    / _ \\  \n");
    printf("  |_|__   _|_|_   /__/_   _\\__|  \\__,_|   /__/_  |_||_|   _|_|_   \\___/  \n");
    printf("_|\"\"\"\"\"|_|\"\"\"\"\"|_|\"\"\"\"\"|_|\"\"\"\"\"|_|\"\"\"\"\"|_|\"\"\"\"\"|_|\"\"\"\"\"|_|\"\"\"\"\"|_|\"\"\"\"\"| \n");
    printf("`'-0-0-''`-0-0-''`-0-0-''`-0-0-''`-0-0-''`-0-0-''`-0-0-''`-0-0-''`-0-0-'  \n");
    printf("                ______\n");
    printf("             .-'      `-.\n");
    printf("           .'            `.\n");
    printf("         .'                `.\n");
    printf("        /    .------.        \\\n");
    printf("       /   .'        `.        \\\n");
    printf("      |   /   ○    ○   \\        |\n");
    printf("      |  |      ∩        |       | \n");
    printf("      |   \\   _______   /        |\n");
    printf("       \\   `.        .'        /\n");
    printf("        `.  `------'        .'\n");
    printf("          `.              .'\n");
    printf("            `-.        .-'\n");
    printf("               `------'\n");
}

void post_order_process(node_t* node) {
    if (node) {
        post_order_process(node->left);
        post_order_process(node->right);
        if (node->type == NODE_COMMAND) {
            process_start(node);
            printf("NODE. ARGV[0]: %s TYPE: %d. Address: %p\n", node->tokens.argv[0], (int) node->type, node);
        } else {
            int left_return = node->left->exit_status;
            int right_return = node->right->exit_status;
            int type = node->type;
            switch (type)
            {
            case NODE_AND:
                node->exit_status = left_return && right_return;
                printf("AND NODE. STATUS: %d. \n", (int)node->exit_status);
                break;
            case NODE_OR:
                node->exit_status = left_return || right_return;
                printf("OR NODE. STATUS: %d. \n", (int)node->exit_status);
                break;
            case NODE_PIPE:
                break;
            default:
                break;
            }
        }
    }
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