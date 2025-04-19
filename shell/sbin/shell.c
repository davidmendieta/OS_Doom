#include <stdbool.h>
#include <sys.h>

void output_prompt() {
    printf("$ ");
}


// read form fd into buffer the next line
char* input_line (char* buffer, int fd) {
    int i = 0;
    read(buffer, 1);
    while(buffer[i] != '\n' || buffer[i] != '\0') {
        read(buffer, 1);
    }
    buffer[i] = '\0';
}

int process_start(char* command) {
    Tokenize* tokens = get_tokens(command);
    char* path = tokens.cmd;
    char** argv = tokens.args;
    int id = fork();
    if(id < 0) {
        printf("can't fork\n");
    } else if(id == 0) {
        // child
        int status = execl(path, argv);
        printf("couldn't execute %s", path);
    } else {
        // parent
        int status = 42;
        wait(id, &status);
        printf("process exited with status %d\n", status);
    }
}


int main(int argc, char** argv) {

    int stdin = open("pathtofile", 0); // our test case

    while (true) // you may want to provide a built-in "exit" command
    {
        char* command = malloc(4096);
        int proc;
        output_prompt();               // display a prompt
        command = input_line(command, stdin);        // get a line of input, which will become the command to execute
        proc = process_start(command); // start a process from the command
        // TODO be able to send input to process and display their output
    }
}