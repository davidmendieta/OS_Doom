#include "libc.h"
#include "ac97.h"

// These come from your AC97 driver

int main(int argc, char** argv) {
    printf("*** launching AC97 audio test\n");

    if (init_ac97() == 0) {
        printf("*** audio init success\n");
    } else {
        printf("*** audio init failed\n");
        shutdown();
        return 1;
    }

    // Wait a bit to give QEMU time to emit sound
    for (volatile int i = 0; i < 100000000; i++) {}

    shutdown_ac97();
    printf("*** audio shutdown success\n");

    shutdown(); // ends the test cleanly
    return 0;
}
