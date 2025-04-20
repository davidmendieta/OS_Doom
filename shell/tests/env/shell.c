#include "shell.h"
#include "tokenizer.c"

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

bool built_in(const char** argv, int argc) {
    const char* path = argv[0];
    if (streq(path, "cd")) {
        if (argc != 2) error("invalid args for command cd\n");
        chdir(argv[1]);
        return false;
    } else if (streq(path, "path")) {
        printf("¯\\_(ツ)_/¯\n");
        return false;
    } else if (streq(path, "exit")) {
        exit(0);
        error("returned from exit\n");
    }
    return true;
}

void process_start(char* command) {
    Tokenized* tokens = tokenize(command);
    const char** argv = (const char**) tokens->argv;
    const char* path = argv[0];
    int argc = tokens->argc;
    if(!built_in(argv, argc)) {
        printf("RETURNING\n");
        return;
    }
    int id = fork();
    if(id < 0) {
        printf("can't fork\n");
    } else if(id == 0) {
        // child
        printf("im forking %s\n", path);
        // if(built_in(argv, argc)) {
            // executable
            printf("im exec\n");
            execl(path, (const char*) argv[0], (const char*) argv[1]);
            printf("couldn't execute %s\n", path);
            exit(0);
        // }
    } else {
        // parent
        uint32_t status = 42;
        wait(id, &status);
        printf("process exited with status %ld\n", status);
    }
}


int main(int argc, char** argv) {

    int stdin = open("/sbin/input", 0); // our test case
    printf("STDIN: %d\n", stdin);
    char *buffer = (void*) malloc(4096);
    read(stdin, (void*) buffer, 4096);
    printf("buffer; %s\n", buffer);

    while (*buffer) // you may want to provide a built-in "exit" command
    {
        char* command = malloc(4096);
        output_prompt();                             // display a prompt
        buffer = input_line(command, buffer);        // get a line of input, which will become the command to execute
        process_start(command);                      // start a process from the command
        free(command);
        printf("loop");
        // TODO be able to send input to process and display their output
    }
}