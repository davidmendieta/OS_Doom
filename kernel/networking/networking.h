#pragma once

#include "interface/network_interface.h"
#include "../shared.h"

extern StrongPtr<NetworkInterface> network_interface;

void networkingMain(void);