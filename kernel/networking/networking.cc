#include "networking.h"

StrongPtr<NetworkInterface> network_interface;

void networkingMain(void) {
    network_interface = StrongPtr<NetworkInterface>::make(19216810);
    Debug::printf("my_ip %x", 19216810);
    // --------------- CLIENT CODE ----------------
     //network_interface->send_arp_request(19216811);

   // while(1){};
    
}