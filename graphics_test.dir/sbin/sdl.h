#include <stdint.h>

// Video mode flags:
#define SDL_SWSURFACE 0x00000000  // Surface is in system memory
#define SDL_HWPALETTE 0x00000001  // Surface has a hardware palette
#define SDL_DOUBLEBUF 0x00000002  // Surface is double buffered
#define SDL_FULLSCREEN 0x00000004 // Surface is fullscreen
#define SDL_OPENGL 0x00000008     // Surface is OpenGL

typedef struct
{
    void *pixels; // Pointer to the actual pixel data
    int w, h;     // Width and height
    int pitch;    // Bytes per row (may be larger than width * bytes per pixel)
    int flags;    // Surface flags
} SDL_Surface;



typedef struct
{
    unsigned char r, g, b, a;
} SDL_Color;

typedef struct
{
    uint32_t red;
    uint32_t green;
    uint32_t blue;
} color_t;


extern int init_video_mode(int width, int height, int bpp, int flags);


extern void *create_surface(int width, int height, int bpp);  


// 3. Pixel data access
extern int lock_surface(void *surface);


extern void unlock_surface(void *surface);


// 4. Buffer swapping
extern int flip_surface(void *surface);


// 5. Palette management (for 8-bit mode)
extern int set_palette(void *surface, color_t *colors, int first, int count);