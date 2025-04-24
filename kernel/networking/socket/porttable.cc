#include "porttable.h"

namespace sockets {

PortTable port_table{};

Atomic<uint16_t> dynamic_port_num{49151};

uint16_t pick_port() {
    // Completely not thread safe but I don't think I need it to be fully thread safe
    for (int i = 0; i <= 65535 - 49151; i++) {
        uint16_t port = dynamic_port_num.fetch_add(1);
        if (port + 1 > 65535) {
            dynamic_port_num.set(49151);
        }
        if (!port_table.contains(port)) {
            return port;
        }
    }
    return 0;
}

}  // namespace sockets