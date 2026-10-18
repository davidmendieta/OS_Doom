/* Adam Schoenberg contributed code to run global constructors */

#include "crt.h"
#include <stdint.h>

typedef void (*func_ptr)();

extern "C" func_ptr _init_array_start[], _init_array_end[];
extern "C" func_ptr _fini_array_start[], _fini_array_end[];
extern "C" func_ptr _ctors_start[], _ctors_end[];

void CRT::init(void)
{
    // Bare-metal GCC's legacy list runs backwards and has sentinel entries.
    for (func_ptr* func = _ctors_end; func != _ctors_start; )
    {
        auto address = reinterpret_cast<uintptr_t>(*--func);
        if (address != 0 && address != UINTPTR_MAX) (*func)();
    }
    for (func_ptr* func = _init_array_start; func != _init_array_end; func++)
    {
        (*func)();
    }
}

void CRT::fini(void)
{
    for (func_ptr* func = _fini_array_start; func != _fini_array_end; func++)
    {
        (*func)();
    }
}

extern "C" void __cxa_atexit() __attribute__((weak));

extern "C" void __cxa_atexit() {
}

