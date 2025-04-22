#include "networking.h"
#include "parser.h"

StrongPtr<NetworkInterface> network_interface;

void networkingMain(void) {
    network_interface = StrongPtr<NetworkInterface>::make();
    // --------------- CLIENT CODE ----------------

    const char test_packet[60] __attribute__((aligned(16))) = {
        0xff,0xff,0xff,0xff,0xff,0xff, // broadcast dest MAC
        0x12,0x34,0x56,0x78,0x9a,0xbc, // source MAC
        0x08,0x00,                     // Ethertype: IPv4
        'h','e','l','l','o'
    };
    network_interface->send(test_packet, 60);

    // --------------- HOST CODE -----------------
    // The following works but we should do it with interrupts instead
    while (1) {
    }
}