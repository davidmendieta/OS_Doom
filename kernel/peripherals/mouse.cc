#include "mouse.h"
#include "io.h"
#include "machine.h"
#include "idt.h"
#include "libk.h"
#include "debug.h"

//allows for data transfer from PS/2 and mouse
#define PS2_DATA_PORT 0x60
//sends commands to the PS/2 controller
//this allows us to enable the mouse and send commands to the mouse
#define PS2_CMD_PORT 0x64

//we initialize the mouse position to 0,0
//REMEMBER! the Y axis is inverted in the mouse 
//AND  0,0 is the top left corner of the screen
static int8_t xAxisMouse = 0;
static int8_t yAxisMouse = 0;

void ps2_mouse_init() {
    //enable the mouse
	//outb will send the command (0xA8) to the PS/2 controller
	//this command enables the mouse port
    outb(PS2_CMD_PORT, 0xA8);

    //starting up the mouse
	//0x20 requests the command byte from teh PS/2 controller
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

    //resets the mouse, WHY DO WE NEED THIS???
	//is used to reset the mouse to its default state
    outb(PS2_DATA_PORT, MOUSE_RESET);
    inb(PS2_DATA_PORT);
}


//TODO: ADD FUNCTIONS TO GET THE MOUSE POSITION WHEN CLICKING AND STUFF LIKE THAT!!!!
void mouseHandler() {
	//create an array of 3 elements to store the mouse data
    static uint8_t packet[3];
    static uint8_t packetIndex = 0;

    //read the data from the PS/2 controller
    packet[packetIndex] = inb(PS2_DATA_PORT);
	packetIndex++;

	if (packetIndex == 1) {
		//checks for the first byte (button states)
		if ((packet[0] & 0x08) == 0) {
			 //this is an invalid packet, reset index
			packetIndex = 0;
			return;
		}
	}
	//once the index hits 3, we know we have a full packet
    if (packetIndex == 3) {
		//packet[0] contains the button states
		//checks if the left, right, and middle buttons are pressed
		// we use bitwise AND to check the button states with the masks
		bool checkLeft = packet[0] & MOUSE_LEFT;
        bool checkRight = packet[0] & MOUSE_RIGHT;
        bool checkMiddle = packet[0] & MOUSE_MIDDLE;

        //getting the x and y movement from the packet
        int8_t xChange = packet[1];
        int8_t yChange = packet[2];
        //updating the mouse position
        xAxisMouse += xChange;
		//Y axis is inverted in the mouse, so we subtract yChange
        yAxisMouse -= yChange; 

        //we are checking the button states now
		//We print a message to tell the user
		//BUT HOW DO WE TELL THE SYSTEM???
		if(checkLeft){
			Debug::printf("Left button has been pressed on the mouse!\n");
		}else if(checkRight){
			Debug::printf("Right button has been pressed on the mouse!\n");
		}else if(checkMiddle){
			Debug::printf("Middle button has been pressed on the mouse!\n");
		}
        packetIndex = 0; 
    }
}

//pretty self explanatory stuff lol
int mouse_get_x() {
    return xAxisMouse;
}

int mouse_get_y() {
    return yAxisMouse;
}

extern "C" void irq12_handler() {//gheith syntax moment
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

	//something about pics and sending EOI, essentially tells PIC we are done with the interrupt
    outb(0x20, 0x20); 
}