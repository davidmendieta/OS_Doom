#include "shell.h"
#include "tokenizer.h"

#define TESTING

void output_prompt() {
    printf("$ ");
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

void handle_fork(const char* path, Tokenized* tokens) {
    int id = fork();
    if (id < 0) {
        error("can't fork\n");
    } else if (id == 0) {
        shell_exec(path, tokens->argv, tokens->argc);
        error("couldn't execute command\n");
    } else {
        // parent
        uint32_t status = 42;
        wait(id, &status);
        printf("process exited with status %ld\n", status);
    }
}

void make_bin_path(char* buffer, const char* argv0) {
    const char* bin = "/bin/";
    buffer[0] = 0;
    strcat(buffer, bin);
    strcat(buffer, argv0);
}

void process_start(char* command) {
    Tokenized tokens;
    tokens_init(&tokens, command);
    tokenize(&tokens, command);

    // built in
    enum CMD_CODES command_id = get_command_id(tokens.argv);
    if (command_id != CMD_EXTERNAL) {
        handle_built_in(command_id, tokens);
        tokens_free(&tokens);
        return;
    }    

    // elf process ( user executable )
    int fd = open(tokens.argv[0], 0);
    uint32_t buffer = 0;
    read(fd, &buffer, sizeof(buffer));
    if (buffer == 0x7f454c46) {
        handle_fork(tokens.argv[0], &tokens);
        tokens_free(&tokens);
        close(fd);
        return;
    }
    close(fd);

    // elf process ( bin executable )
    const char* bin = "/bin/";
    int length = strlen(tokens.argv[0]);
    char* path = (char*) malloc(sizeof(bin) + length + 1);
    make_bin_path(path, tokens.argv[0]);
    fd = open(path, 0);
    if (fd >= 0) {
        handle_fork(path, &tokens);
        tokens_free(&tokens);
        free(path);
        close(fd);
        return;
    }
    free(path);
    close(fd);

    // pistash script
    fd = open(tokens.argv[0], 0);
    if (fd >= 0) {
        char shebang[14]; 
        read(fd, shebang, 14); // read first 14 bytes of the file (13 + \n)
        shebang[13] = '\0'; 
        if (streq(shebang, "#!bin/pistash")) { // when we have more interpreters, replace with check for #!
            char *buffer = (void*) malloc(4096);
            read(fd, (void*) buffer, 4096);
            while (*buffer != '\0') {
                char* command = malloc(4096);
                buffer = input_line(command, buffer);
                process_start(command);
                free(command);
            }
            free(buffer);
        }
    }
    close(fd);

    tokens_free(&tokens);
}

int main(int argc, char** argv) {
    char* buffer;

    #ifdef TESTING
    int stdin = open("/user/input", 0); // our test case
    buffer = (void*) malloc(4096);
    read(stdin, (void*) buffer, 4096);
    #endif

    while (true) {
        char* command = malloc(4096);
        output_prompt();                             // display a prompt
        buffer = input_line(command, buffer);        // get a line of input, which will become the command to execute
        process_start(command);                      // start a process from the command
        free(command);
        // TODO be able to send input to process and display their output
    }
}