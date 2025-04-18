#pragma once

#include "debug.h"

#define PCI_INTERRUPT_LINE 0x3C
#define PCI_COMMAND 0x04
#define PCI_BAR0 0x10

#define CONFIG_ADDRESS 0xCF8
#define CONFIG_DATA    0xCFC
#define NUM_BARS 6 // FIXME : is 6 correct?
#define NUM_BUS 256

typedef struct {
    uint8_t bus;
    uint8_t slot;
    uint8_t function;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t header;
    uint32_t bars[NUM_BARS];
} PciDevice;

extern PciDevice pciDevices[NUM_BUS];
extern size_t deviceCount;

// bus = the PCI bus number
// slot (device) = the PCI slot number
// function = is the function number
// register_offset = the offset in the configuration space
extern uint8_t pciConfigRead8(uint8_t bus, uint8_t slot, uint8_t function, uint8_t register_offset);

extern uint16_t pciConfigRead(uint8_t bus, uint8_t slot, uint8_t function, uint8_t register_offset);

// writes to a config space, similar to read above.
extern void pciConfigWrite(uint8_t bus, uint8_t slot, uint8_t function, uint8_t register_offset, uint16_t value);

// checks if a PCI device exists at the given bus and slot
// returns the vendor ID
extern uint16_t checkVendor(uint8_t bus, uint8_t slot);

// finds Device by Vendor & Device ID
extern PciDevice* findDeviceVD(uint16_t vendor_id, uint16_t device_id);

// enumerates all PCI devices
extern void pciEnumDevices();

