#include "shell.h"
#include "tokenizer.h"

void output_prompt() {
    printf("$ ");
}

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
    return (src + i + 1);
}

// read keyboard input into buffer
// return on newline
// void input_line(char* buffer) {
//     uint32_t write = 0;
//     char c = 0; // read from keyboard
//     while (!is_newline(c)) {
//         buffer[write] = c;
//         c = 0; // read from keyboard
//         write++;
//     }
//     buffer[write] = 0;
// }

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
            for (int i = 1; tokens.argv[i]; i++) {
                printf("%s ", tokens.argv[i]);
            }
            printf("\n");
            break;
        case CMD_PWD:
            // todo ask for a syscall
            printf("i think you are in a directory ngl\n");
            break;
        default:
            error("how did you get here??\n");
    }
}

void process_start(char* command) {
    Tokenized tokens;
    tokens_init(&tokens, command);
    tokenize(&tokens, command);

    enum CMD_CODES command_id = get_command_id(tokens.argv);
    if (command_id != CMD_EXTERNAL) {
        handle_built_in(command_id, tokens);
        return;
    }

    int id = fork();
    if(id < 0) {
        error("can't fork\n");
    } else if(id == 0) {
        printf("executing instruction: %s\n", tokens.argv[0]);
        // executable
        execl(tokens.argv[0], (const char*) tokens.argv[0], (const char*) tokens.argv[1]);
        error("couldn't execute command\n");
    } else {
        // parent
        uint32_t status = 42;
        wait(id, &status);
        printf("process exited with status %ld\n", status);
    }
    tokens_free(&tokens);
}

int main(int argc, char** argv) {
    int stdin = open("/sbin/input", 0); // our test case
    char *buffer = (void*) malloc(4096);
    read(stdin, (void*) buffer, 4096);

    while (true) {
        char* command = malloc(4096);
        output_prompt();                             // display a prompt
        buffer = input_line(command, buffer);        // get a line of input, which will become the command to execute
        process_start(command);                      // start a process from the command
        free(command);
        // TODO be able to send input to process and display their output
    }
}