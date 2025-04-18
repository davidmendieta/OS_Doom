#include "networking.h"
#include <smp.h>

StrongPtr<RTL8139> rtl8139;

void networkingMain(void) {
    // --------------- CLIENT CODE ----------------
    rtl8139 = StrongPtr<RTL8139>::make();

    uint8_t test_packet[60] __attribute__((aligned(16))) = {
        0xff,0xff,0xff,0xff,0xff,0xff, // broadcast dest MAC
        0x12,0x34,0x56,0x78,0x9a,0xbc, // source MAC
        0x08,0x00,                     // Ethertype: IPv4
        'h','e','l','l','o'
    };
    rtl8139->send_packet(test_packet, 60);
    // while (1) {
    //     // do nothing — wait for interrupt
    // }

    // --------------- HOST CODE -----------------
    // The following works but we should do it with interrupts instead
    while (1) {
    }
}