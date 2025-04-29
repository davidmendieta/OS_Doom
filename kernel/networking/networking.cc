#include "networking.h"

StrongPtr<NetworkInterface> network_interface;

void networkingMain(void) {
    network_interface = StrongPtr<NetworkInterface>::make(19216812); // My IP address

    // network_interface->send_ip_udp("Hello", 5, 19216812, 1234, 19216812, 4321, nullptr);
    while(1){
    };

}

