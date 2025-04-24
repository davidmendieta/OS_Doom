#include "networking.h"

StrongPtr<NetworkInterface> network_interface;

void networkingMain(void) {
    network_interface = StrongPtr<NetworkInterface>::make(19216811);
    // --------------- CLIENT CODE ----------------
    // network_interface->send_arp_request(19216811);

    //while(1){};
    
}