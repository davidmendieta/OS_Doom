#include <stdint.h>

#include "sys.h"
#include "stdint.h"
#include "debug.h"
#include "idt.h"
#include "machine.h"
#include "sdl.h"

#define COLOR_RED 0x4

int main(int argc, char **argv)
{

	SDL_Surface *screen = SDL_SetVideoMode(320, 200, 16, SDL_SWSURFACE);

	SDL_init();

	uint8_t *pixels = (uint8_t *)screen->pixels;

	for (uint32_t y = 0; y < screen->h; y++)
	{
		for (uint32_t x = 0; x < screen->w; x++)
		{
			size_t offset = y * screen->pitch + x;
			pixels[offset] = COLOR_RED;
		}
	}

	SDL_Flip(screen);

	return 0;
}


void heap_init(void)
{
	// ...
}