#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <os/runtime.h>
#include "SDL.h"

extern int doom_main(int argc, char **argv);
int os_input_testing;
unsigned os_net_role, os_net_ip, os_net_server;
extern void I_PumpSound(void);
extern void I_ServerPoll(void);

uint32_t SDL_GetTicks(void) { return os_milliseconds(); }
void SDL_Delay(uint32_t milliseconds) {
    uint32_t start = os_milliseconds();
    do { I_PumpSound(); I_ServerPoll(); os_yield(); } while ((uint32_t)(os_milliseconds() - start) < milliseconds);
}
int main(void) {
    os_input_testing = access("/etc/input-test", F_OK) == 0;
    char *args[] = {"prboom", "-iwad", "/doom/doom1.wad",
                    "-width", "320", "-height", "200", "-vidmode", "8bit", NULL, NULL, NULL, NULL};
    int argc = 9;
    if (access("/etc/no-music", F_OK) == 0) args[argc++] = "-nomusic";
    FILE *network = fopen("/etc/network", "r");
    if (network) {
        if (fscanf(network, "%u %x %x", &os_net_role, &os_net_ip, &os_net_server) != 3 ||
            (os_net_role != 1 && os_net_role != 2)) {
            fputs("Invalid /etc/network configuration\n", stderr);
            return 1;
        }
        fclose(network);
        args[argc++] = "-net";
        args[argc++] = "native";
    }
    setenv("HOME", "/", 1);
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    return doom_main(argc, args);
}
