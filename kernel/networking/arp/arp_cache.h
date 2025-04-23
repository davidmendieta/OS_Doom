#pragma once

#include "../../promise_with_timeout.h"

class ARPCache {
    
    
    public:
    StrongPtr<PromiseWithTimeout> p[100];
    
    char*  get(uint32_t ip);


        // You also need to take care of setting the value of p[ip] (and making sure it's not a nullptr) to the actual mac address once you receive an ARP response}
};