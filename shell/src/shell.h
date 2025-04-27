#pragma once
#include "sys.h"
#include "libc.h"

struct Command {
    const char* name;
    int code;
} typedef Command;

enum CMD_CODES {
    CMD_IGNORE,     // comments
    CMD_CD,         // cd
    CMD_PATH,       // path??
    CMD_EXIT,       // exit with status 0
    CMD_ECHO,       // prints
    CMD_PWD,        // print cwd
    CMD_FALSE,      // set exit status to 1 in command chain
    CMD_TRUE,       // set exit status to 0 in command chain
    CMD_LS,         // ls
    CMD_SLEEP,      // sleep
    CMD_SET,        // set command for shell variables
    CMD_UNSET,      // remove command for shell variables
    CMD_GET,        // get command for shell variables
    CMD_EXTERNAL,   // external command
    CMD_PRINT_NUT,  // print nut
    CMD_GET_NUT_FACT // print nut fact
};

Command commands[] = {
    {"cd", CMD_CD},
    {"path", CMD_PATH},
    {"exit", CMD_EXIT},
    {"echo", CMD_ECHO},
    {"pwd", CMD_PWD},
    {"false", CMD_FALSE},
    {"true", CMD_TRUE},
    {"ls", CMD_LS},
    {"sleep", CMD_SLEEP},
    {"set", CMD_SET},
    {"unset", CMD_UNSET},
    {"get", CMD_GET},
    {"logo", CMD_PRINT_NUT},
    {"nut_fact", CMD_GET_NUT_FACT},
    {NULL, CMD_EXTERNAL}  // default
};

void error(const char* msg) {
    printf("%s", msg);
    exit(-1);
}

int get_command_id(char* argv0) {
    if (argv0[0] == '#') return CMD_IGNORE;
    for (int i = 0; commands[i].name != NULL; i++) {
        if (streq(argv0, commands[i].name)) return commands[i].code;
    }
    return CMD_EXTERNAL;
}

char* tokenize_and_process(char*);