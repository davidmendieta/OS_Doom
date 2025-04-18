#pragma once

#include <stdint.h>
#include "debug.h"



namespace PS2 {

	extern void init();

	extern uint8_t read_ps2_data();

	extern uint8_t read_ps2_status();

	extern uint8_t send_ps2_command();

	extern bool ps2_can_read();

	extern bool ps2_reset_device();
}
