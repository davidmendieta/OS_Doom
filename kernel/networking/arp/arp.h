
#pragma once


#include "../../shared.h"
extern void send_arp(uint8_t* ip, uint8_t opcode);


extern void receive_arp(uint8_t* packet, int len);