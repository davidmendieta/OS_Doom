// keyboard.h
#pragma once

#include <stddef.h>

void keyboard_init();
int keyboard_read(char* buf, size_t count);
