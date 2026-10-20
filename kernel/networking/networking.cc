#include "networking.h"
#include "udp.h"

// Historical stack remains available as source; native UDP owns the NIC.
StrongPtr<NetworkInterface> network_interface;
void networkingMain(void) { NativeNet::init(); }
