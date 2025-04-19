#include <stdbool.h>
#include "sys.h"
#include "tokenizer.c"

// Built in commands 
// - cd (one arg)
// - path (any num args)
// - exit (no args)

int streq(const char* s1, const char* s2) {
    while (*s1 && *s2) {
        if (*s1 != *s2) return 0;
        s1++;
        s2++;
    }
    return *s1 == *s2;
}

void output_prompt() {
    printf("$ ");
}


// read form fd into buffer the next line
char* input_line (char* buffer, int fd) {
    int i = 0;
    read(fd, buffer, 1);
    while(buffer[i] != '\n' || buffer[i] != '\0') {
        read(fd, buffer, 1);
    }
    buffer[i] = '\0';
}

bool built_in(const char** argv, int argc) {
    const char* path = argv[0];
    if(streq(path, "cd")) {
        if(argc != 2) {
            printf("invalid args for command cd\n");
        }
        chdir(argv[1]);
        return true;
    }  
    if(streq(path, "path")) {
        printf("¯\\_(ツ)_/¯\n");
        return true;
    }
    if(streq(path, "exit")) {
        if(argc != 1) {
            printf("invalid args for exit cd\n");
        }
        exit(0);
        return true;
    }
    return false;
}

void process_start(char* command) {
    Tokenized* tokens = tokenize(command);
    const char** argv = (const char**) tokens->argv;
    const char* path = argv[0];
    int argc = tokens->argc;
    int id = fork();
    if(id < 0) {
        printf("can't fork\n");
    } else if(id == 0) {
        // child
        if(!built_in(argv, argc)) {
            // executable
            execl(path, (const char*) argv[0], (const char*) argv[1]);
            printf("couldn't execute %s", path);
        }
    } else {
        // parent
        uint32_t status = 42;
        wait(id, &status);
        printf("process exited with status %ld\n", status);
    }
}


int main(int argc, char** argv) {

    int stdin = open("pathtofile", 0); // our test case

    while (true) // you may want to provide a built-in "exit" command
    {
        char* command = malloc(4096);
        output_prompt();               // display a prompt
        command = input_line(command, stdin);        // get a line of input, which will become the command to execute
        process_start(command); // start a process from the command
        // TODO be able to send input to process and display their output
    }
}