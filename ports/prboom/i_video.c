/* Native mode-13h backend for the PrBoom software renderer. */
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <os/video.h>
#include "doomstat.h"
#include "i_system.h"
#include "i_main.h"
#include "lprintf.h"
#include "i_video.h"
#include "r_draw.h"
#include "st_stuff.h"
#include "w_wad.h"

int use_doublebuffer, use_fullscreen, desired_fullscreen, leds_always_off;
int usejoystick, joyleft, joyright, joyup, joydown;
static int video_fd = -1;
extern void I_InputReport(void);

void I_PreInitGraphics(void) {}
void I_CalculateRes(unsigned int width, unsigned int height) {
    (void)width; (void)height;
    SCREENWIDTH = SCREENPITCH = OS_VIDEO_WIDTH;
    SCREENHEIGHT = OS_VIDEO_HEIGHT;
}
void I_SetRes(void) {
    I_CalculateRes(320, 200);
    for (int i = 0; i < 5; i++) {
        screens[i].width = SCREENWIDTH;
        screens[i].height = i == 4 ? ST_SCALED_HEIGHT + 1 : SCREENHEIGHT;
        screens[i].byte_pitch = SCREENPITCH;
        screens[i].short_pitch = SCREENPITCH / 2;
        screens[i].int_pitch = SCREENPITCH / 4;
    }
}
void I_UpdateVideoMode(void) {
    V_InitMode(VID_MODE8);
    V_FreeScreens();
    I_SetRes();
    V_AllocScreens();
    R_InitBuffer(SCREENWIDTH, SCREENHEIGHT);
    if (os_video_init() < 0) I_Error("VGA initialization failed");
    if (video_fd < 0) video_fd = open("/dev/vga", O_WRONLY);
    if (video_fd < 0) I_Error("Cannot open /dev/vga");
    I_SetPalette(0);
}
void I_InitGraphics(void) { I_UpdateVideoMode(); }
void I_ShutdownGraphics(void) { if (video_fd >= 0) close(video_fd); video_fd = -1; }
void I_SetPalette(int palette) {
    int lump = W_GetNumForName("PLAYPAL");
    if (palette < 0 || (palette + 1) * 768 > W_LumpLength(lump)) I_Error("Bad palette");
    const byte *data = W_CacheLumpNum(lump);
    int result = os_video_set_palette((const os_video_color *)(data + palette * 768), 0, 256);
    W_UnlockLumpNum(lump);
    if (result < 0) I_Error("Palette update failed");
}
extern void I_PumpSound(void);
void I_FinishUpdate(void) {
    I_PumpSound();
    I_InputReport();
    static int announced;
    static int level_announced;
    if (write(video_fd, screens[0].data, OS_VIDEO_FRAME_BYTES) != OS_VIDEO_FRAME_BYTES)
        I_Error("Frame write failed");
    if (!announced && gamestate == GS_DEMOSCREEN) {
        announced = 1;
        puts("*** doom: title frame submitted");
    }
    if (!level_announced && gamestate == GS_LEVEL) {
        level_announced = 1;
        puts("*** doom: level frame submitted");
    }
}
void I_UpdateNoBlit(void) {}
void I_StartFrame(void) {}
int I_ScreenShot(const char *name) { (void)name; return -1; }
