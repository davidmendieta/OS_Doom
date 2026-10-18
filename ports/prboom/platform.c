#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <os/runtime.h>
#include "SDL.h"

extern int doom_main(int argc, char **argv);
int os_input_testing;

uint32_t SDL_GetTicks(void) { return os_milliseconds(); }
void SDL_Delay(uint32_t milliseconds) {
    uint32_t start = os_milliseconds();
    do { os_yield(); } while ((uint32_t)(os_milliseconds() - start) < milliseconds);
}
int main(void) {
    os_input_testing = access("/etc/input-test", F_OK) == 0;
    char *args[] = {"prboom", "-iwad", "/doom/doom1.wad", "-nosound",
                    "-width", "320", "-height", "200", "-vidmode", "8bit", NULL};
    setenv("HOME", "/", 1);
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    return doom_main(sizeof(args) / sizeof(args[0]) - 1, args);
}
