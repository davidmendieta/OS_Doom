#pragma once
#include "../../include/os/net.h"
namespace NativeNet {
void init();
int configure(uint32_t ip);
int open(unsigned port);
int send(int socket, const os_datagram* packet);
int receive(int socket, os_datagram* packet);
int close(int socket);
int stats(os_net_stats* out);
}
