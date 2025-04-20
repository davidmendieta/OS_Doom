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

bool handle_built_in(int argc, char** argv) {
    const char* path = argv[0];
    if (streq(path, "cd")) {
        if (argc != 2) error("invalid args for command cd\n");
        chdir(argv[1]);
        return false;
    } else if (streq(path, "path")) {
        printf("¯\\_(ツ)_/¯\n");
        return false;
    } else if (streq(path, "exit")) {
        shutdown();
    }
    return true;
}

void process_start(char* command) {
    Tokenized tokens;
    tokens_init(&tokens, command);
    tokenize(&tokens, command);

    if(!handle_built_in(tokens.argc, tokens.argv)) {
        printf("RETURNING\n");
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

    while (true) // you may want to provide a built-in "exit" command
    {
        printf("buffer %s\n", buffer);
        char* command = malloc(4096);
        output_prompt();                             // display a prompt
        buffer = input_line(command, buffer);        // get a line of input, which will become the command to execute
        process_start(command);                      // start a process from the command
        free(command);
        printf("loop");
        // TODO be able to send input to process and display their output
    }
}