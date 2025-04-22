#include "../drivers/rtl8139.h"

class NetworkInterface {
    RTL8139 rtl8139;
    uint32_t my_IP;
};