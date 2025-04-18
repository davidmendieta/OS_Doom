#!/bin/bash
# Launch QEMU with VGA device and graphical output
# Usage: ./run_qemu_vga.sh

# Path to your kernel or OS image. Adjust as needed.
KERNEL_IMG="build/kernel.img"

# Check if DISPLAY is set for X11 forwarding
if [ -z "$DISPLAY" ]; then
    echo "Warning: DISPLAY is not set. If you are SSH'd in, use 'ssh -X' or 'ssh -Y' for X11 forwarding."
    echo "Alternatively, use QEMU's VNC or SDL output."
fi

# Launch QEMU with VGA and enable SDL display
# QEMU with VNC display (default)
qemu-system-i386 \
    -kernel "$KERNEL_IMG" \
    -device VGA \
    -display vnc=:1 \
    -m 128M \
    -serial stdio

# To view the display:
# 1. On your local machine, run:
#    vncviewer <server>:5901
#    (replace <server> with the hostname or IP of your lab machine)
#
# 2. If you can't connect directly, use SSH port forwarding:
#    ssh -L 5901:localhost:5901 <your_lab_user>@<your_lab_host>
#    Then, on your local machine:
#    vncviewer localhost:5901

