#include <stdbool.h>
#include <stddef.h>
#include "sys.h"
#include "libc.h"

struct Command {
    const char* name;
    int code;
} typedef Command;

enum CMD_CODES {
    CMD_IGNORE,
    CMD_CD,
    CMD_PATH,
    CMD_EXIT,
    CMD_ECHO,
    CMD_PWD,
    CMD_EXTERNAL
};

Command commands[] = {
    {"cd", CMD_CD},
    {"path", CMD_PATH},
    {"exit", CMD_EXIT},
    {"echo", CMD_ECHO},
    {"pwd", CMD_PWD},
    {NULL, CMD_EXTERNAL}  // default
};

void error(const char* msg) {
    printf("%s", msg);
    exit(-1);
}

int get_command_id(char** argv) {
    const char* cmd = argv[0];
    if (cmd[0] == '#') return CMD_IGNORE;
    for (int i = 0; commands[i].name != NULL; i++) {
        if (streq(cmd, commands[i].name)) return commands[i].code;
    }
    return CMD_EXTERNAL;
}