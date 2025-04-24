#include <stdint.h>

#include "sys.h"
#include "stdint.h"
#include "debug.h"
#include "idt.h"
#include "machine.h"
#include "sdl.h"




int main(int argc, char **argv)
{
  SDL_Surface *screen = SDL_SetVideoMode(320, 200, 16, SDL_SWSURFACE);



  return 0;
}

// This does nothing.
//
// It's just so I don't have to change a line in `crt0.S`.
//
// Though, this is kinda interesting...
// Why would the C runtime need this?
void heap_init(void)
{
  // ...
}