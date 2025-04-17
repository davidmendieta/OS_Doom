#pragma once

#include "drivers/rtl8139.h"
#include "../shared.h"

extern StrongPtr<RTL8139> rtl8139;

void networkingMain(void);