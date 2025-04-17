#include "networking.h"

StrongPtr<RTL8139> rtl8139 = StrongPtr<RTL8139>::make();

void networkingMain(void) {
    while(true) {
        char buffer[1];
        rtl8139->send_packet(buffer, 1);
    }
}