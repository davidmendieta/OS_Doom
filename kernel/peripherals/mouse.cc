#include "mouse.h"
#include "io.h"
#include "machine.h"
#include "idt.h"
#include "libk.h"
#include "debug.h"

namespace mouse {


//allows for data transfer from PS/2 and mouse
#define PS2_DATA_PORT 0x60
//sends commands to the PS/2 controller
//this allows us to enable the mouse and send commands to the mouse
#define PS2_CMD_PORT 0x64

// Add screen dimensions to keep mouse within bounds
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

// Additional mouse commands
#define MOUSE_SET_SAMPLE 0xF3
#define MOUSE_SET_RESOLUTION 0xE8
#define MOUSE_ENABLE_DATA_REPORTING 0xF4
#define MOUSE_DISABLE_DATA_REPORTING 0xF5
#define MOUSE_ENABLE_SCALING 0xE7
#define MOUSE_DISABLE_SCALING 0xE6
#define MOUSE_ALWAYS_SET 0x08
#define MOUSE_X_OVERFLOW 0x40
#define MOUSE_Y_OVERFLOW 0x80
#define MOUSE_X_NEG 0x10
#define MOUSE_Y_NEG 0x20

//we initialize the mouse position to 0,0
//REMEMBER! the Y axis is inverted in the mouse 
//AND  0,0 is the top left corner of the screen
static int8_t xAxisMouse = 0;
static int8_t yAxisMouse = 0;
static bool leftButtonPressed = false;
static bool rightButtonPressed = false;
static bool middleButtonPressed = false;
static float mouseSensitivity = 1.0;

// Helper function to write to mouse and expect acknowledgment
static void mouse_write_and_ack(uint8_t command) {
    // Send command to mouse
    outb(PS2_CMD_PORT, 0xD4);  // Tell PS/2 controller we're sending to mouse
    outb(PS2_DATA_PORT, command);
    // Read acknowledgment byte
    inb(PS2_DATA_PORT);        // Should be 0xFA (ACK)
}

void ps2_mouse_init() {
    // Reset mouse position and button states
    xAxisMouse = 0;
    yAxisMouse = 0;
    leftButtonPressed = false;
    rightButtonPressed = false;
    middleButtonPressed = false;

    //enable the mouse
    //outb will send the command (0xA8) to the PS/2 controller
    //this command enables the mouse port
    outb(PS2_CMD_PORT, 0xA8);

    //starting up the mouse
    //0x20 requests the command byte from the PS/2 controller
    outb(PS2_CMD_PORT, 0x20); 
    //we then read the byte passed in from the PS/2 controller
    //this byte is stored
    uint8_t byteWeReadIn = inb(PS2_DATA_PORT);
    //here we enable IRQ12, which is the mouse interrupt
    //when this is enabled, the mouse will send an interrupt to the CPU
    //when the mouse sends an interrupt, the CPU will call the mouse handler
    //we do this by setting the 2nd bit of the byte we read in byte
    //this sets up the interrupts for future use
    byteWeReadIn |= 0x02;
    //tell the PS/2 controller that the next byte is written, and the next byte will be a command
    //basically, we are telling the PS/2 controller that we are going to send a command
    outb(PS2_CMD_PORT, 0x60);
    //write the byte we read in to the PS/2 controller, this enables the mouse and interrupts
    //this is done by PS/2 controller to send mouse interrupts to the CPU
    outb(PS2_DATA_PORT, byteWeReadIn);

    // Set defaults for better performance
    mouse_set_sample_rate(80);
    mouse_set_resolution(0x00); // One unit per mm
    mouse_disable_scaling();    // Disable acceleration

    // Enable data reporting (start sending mouse packets)
    mouse_enable_data_reporting();

    // Reset the mouse to clear any pending state
    outb(PS2_DATA_PORT, MOUSE_RESET);
    inb(PS2_DATA_PORT);

    Debug::printf("Mouse initialized\n");
}

void mouseHandler() {
    //create an array of 3 elements to store the mouse data
    static uint8_t packet[3];
    static uint8_t packetIndex = 0;

    //read the data from the PS/2 controller
    packet[packetIndex] = inb(PS2_DATA_PORT);
    packetIndex++;

    if (packetIndex == 1) {
        //checks for the first byte (button states)
        if ((packet[0] & MOUSE_ALWAYS_SET) == 0) {
            //this is an invalid packet, reset index
            packetIndex = 0;
            return;
        }
    }
    //once the index hits 3, we know we have a full packet
    if (packetIndex == 3) {
        // Packets with X or Y overflow are probably garbage
        if (packet[0] & MOUSE_X_OVERFLOW || packet[0] & MOUSE_Y_OVERFLOW) {
            packetIndex = 0;
            return;
        }

        //packet[0] contains the button states
        //checks if the left, right, and middle buttons are pressed
        // we use bitwise AND to check the button states with the masks
        bool checkLeft = packet[0] & MOUSE_LEFT;
        bool checkRight = packet[0] & MOUSE_RIGHT;
        bool checkMiddle = packet[0] & MOUSE_MIDDLE;

        // Update button states
        leftButtonPressed = checkLeft;
        rightButtonPressed = checkRight;
        middleButtonPressed = checkMiddle;

        //getting the x and y movement from the packet
        int8_t xChange = packet[1];
        int8_t yChange = packet[2];

        // Handle sign bit for x/y
        if (packet[0] & MOUSE_X_NEG) {
            xChange |= 0xFFFFFF00;
        }
        if (packet[0] & MOUSE_Y_NEG) {
            yChange |= 0xFFFFFF00;
        }

        // Apply sensitivity
        xAxisMouse += (int)(xChange * mouseSensitivity);
        yAxisMouse -= (int)(yChange * mouseSensitivity); // Y is inverted

        // Keep mouse within screen boundaries
        if (xAxisMouse < 0) xAxisMouse = 0;
        if (xAxisMouse >= SCREEN_WIDTH) xAxisMouse = SCREEN_WIDTH - 1;
        if (yAxisMouse < 0) yAxisMouse = 0;
        if (yAxisMouse >= SCREEN_HEIGHT) yAxisMouse = SCREEN_HEIGHT - 1;

        // Debug mouse position and buttons if needed
        if (leftButtonPressed || rightButtonPressed || middleButtonPressed) {
            Debug::printf("Mouse: x=%d, y=%d, L=%d, R=%d, M=%d\n", 
                xAxisMouse, yAxisMouse, 
                leftButtonPressed, rightButtonPressed, middleButtonPressed);
        }

        packetIndex = 0;
    }
}

// Set mouse sample rate (how often the mouse sends updates)
void mouse_set_sample_rate(uint8_t rate) {
    // Write command to mouse device
    outb(PS2_CMD_PORT, 0xD4);
    outb(PS2_DATA_PORT, MOUSE_SET_SAMPLE);
    inb(PS2_DATA_PORT);        // Acknowledge
    
    // Send the rate
    outb(PS2_CMD_PORT, 0xD4);
    outb(PS2_DATA_PORT, rate);
    inb(PS2_DATA_PORT);        // Acknowledge
}

// Set mouse resolution (movement precision)
void mouse_set_resolution(uint8_t resolution) {
    // Write command to mouse device
    outb(PS2_CMD_PORT, 0xD4);
    outb(PS2_DATA_PORT, MOUSE_SET_RESOLUTION);
    inb(PS2_DATA_PORT);        // Acknowledge
    
    // Send the resolution
    outb(PS2_CMD_PORT, 0xD4);
    outb(PS2_DATA_PORT, resolution);
    inb(PS2_DATA_PORT);        // Acknowledge
}

// Enable mouse data reporting (start sending movement packets)
void mouse_enable_data_reporting() {
    outb(PS2_CMD_PORT, 0xD4);
    outb(PS2_DATA_PORT, MOUSE_ENABLE_DATA_REPORTING);
    inb(PS2_DATA_PORT);        // Acknowledge
}

// Disable mouse data reporting (stop sending movement packets)
void mouse_disable_data_reporting() {
    outb(PS2_CMD_PORT, 0xD4);
    outb(PS2_DATA_PORT, MOUSE_DISABLE_DATA_REPORTING);
    inb(PS2_DATA_PORT);        // Acknowledge
}

// Enable mouse acceleration
void mouse_enable_scaling() {
    outb(PS2_CMD_PORT, 0xD4);
    outb(PS2_DATA_PORT, MOUSE_ENABLE_SCALING);
    inb(PS2_DATA_PORT);        // Acknowledge
}

// Disable mouse acceleration
void mouse_disable_scaling() {
    outb(PS2_CMD_PORT, 0xD4);
    outb(PS2_DATA_PORT, MOUSE_DISABLE_SCALING);
    inb(PS2_DATA_PORT);        // Acknowledge
}

// Set mouse sensitivity multiplier
void mouse_set_sensitivity(float sensitivity) {
    mouseSensitivity = sensitivity;
}

// Get current mouse X position
int mouse_get_x() {
    return xAxisMouse;
}

// Get current mouse Y position
int mouse_get_y() {
    return yAxisMouse;
}

// Check if left mouse button is pressed
bool mouse_left_pressed() {
    return leftButtonPressed;
}

// Check if right mouse button is pressed
bool mouse_right_pressed() {
    return rightButtonPressed;
}

// Check if middle mouse button is pressed
bool mouse_middle_pressed() {
    return middleButtonPressed;
}

// Print current mouse status for debugging
void mouse_print_status() {
    Debug::printf("Mouse: x=%d, y=%d, L=%d, R=%d, M=%d\n", 
                 xAxisMouse, yAxisMouse, 
                 leftButtonPressed, rightButtonPressed, middleButtonPressed);
}

extern "C" void irq12_handler() {
    /*Mouse TimeLine:
    (straight from chat)
    The PS/2 mouse sends a data packet (3 bytes) to the PS/2 controller.

    PS/2 Controller Triggers IRQ12:
    The PS/2 controller signals the CPU via IRQ12 that data is ready to be processed.

    CPU Executes irq12_handler:
    The CPU pauses its current task and executes the irq12_handler function:

    Mouse Data Processed:
    The ps2_mouse_handler function reads the data from the PS/2 controller and processes it:
    */
    mouseHandler();

    // Send End-Of-Interrupt signal to both PICs
    outb(0xA0, 0x20); // Send EOI to slave PIC
    outb(0x20, 0x20); // Send EOI to master PIC
}


void test_mouse() {
    // Initialize the mouse
    ps2_mouse_init();
    
    Debug::printf("Mouse test started. Move your mouse to see coordinates.\n");
    
    // Previous position values to detect changes
    int prev_x = -1;
    int prev_y = -1;
    bool prev_left = false;
    bool prev_right = false;
    bool prev_middle = false;
    
    // Loop and print mouse position when it changes
    while (true) {
        int x = mouse_get_x();
        int y = mouse_get_y();
        bool left = mouse_left_pressed();
        bool right = mouse_right_pressed();
        bool middle = mouse_middle_pressed();
        
        // Print only when position or button state changes
        if (x != prev_x || y != prev_y || 
            left != prev_left || right != prev_right || middle != prev_middle) {
            Debug::printf("Mouse: x=%d, y=%d, L=%d, R=%d, M=%d\n",
                         x, y, left, right, middle);
            
            prev_x = x;
            prev_y = y;
            prev_left = left;
            prev_right = right;
            prev_middle = middle;
        }
        
        // Small delay to avoid flooding the console
        for (volatile int i = 0; i < 100000; i++);
    }
}
}