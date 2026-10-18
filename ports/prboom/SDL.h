/* Only the timer API used by upstream SDL/i_system.c is needed here. */
#ifndef OS_PRBOOM_TIMER_H
#define OS_PRBOOM_TIMER_H
#include <stdint.h>
uint32_t SDL_GetTicks(void);
void SDL_Delay(uint32_t milliseconds);
#endif
