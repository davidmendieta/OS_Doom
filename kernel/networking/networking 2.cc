#include "networking.h"

StrongPtr<NetworkInterface> network_interface;

void networkingMain(void) {
    network_interface = StrongPtr<NetworkInterface>::make(19216810);
  
}