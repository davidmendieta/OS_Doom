
#pragma once

#include "../drivers/rtl8139.h"
#include "../../shared.h"
extern void send_arp(uint8_t* ip, uint8_t* mac);