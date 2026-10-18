#include "os/video.h"

    .text
    .global os_video_init
os_video_init:
    mov $OS_SYS_VIDEO_INIT, %eax
    int $48
    ret

    .global os_video_set_palette
os_video_set_palette:
    mov $OS_SYS_VIDEO_PALETTE, %eax
    int $48
    ret

    .section .note.GNU-stack,"",@progbits
