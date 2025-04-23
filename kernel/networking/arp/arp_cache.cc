#include "arp_cache.h"
#include "../interface/network_interface.h"

extern StrongPtr<NetworkInterface> network_interface;

// class ArpCache {

//     StrongPtr<PromiseWithTimeout> p[100]; // Assuming a fixed size for simplicity`
//     public:

char *ARPCache::get(uint32_t ip)
{ // if p[ip] is null: send arp request (might need to do in a .cc to avoid circular dependencies), p[ip] = StrongPtr make PromiseWithTimeout(1);
    // val = p[ip]->get()
    // if(val == nullptr) p[ip] = nullptr
    // else return val

    uint32_t masked_ip_index = ip & 0xFF; // Assuming a simple hash function for the sake of this example
    if (p[masked_ip_index] == nullptr)
    {
        network_interface->send_arp_request(ip);
    }
    Debug::printf("Getting MAC address for %x\n", ip);
    p[ip && 0xFF] = StrongPtr<PromiseWithTimeout>::make(1); // 1 second timeout

    // Send ARP request
    char *val = p[ip && 0xFF]->get();

    if (val == nullptr)
    {
        p[ip] = nullptr;
        return nullptr;
    }
    return val;
}

//     // You also need to take care of setting the value of p[ip] (and making sure it's not a nullptr) to the actual mac address once you receive an ARP response}
// };