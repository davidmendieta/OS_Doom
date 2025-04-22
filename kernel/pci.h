#pragma once

#include "debug.h"

#define PCI_INTERRUPT_LINE 0x3C
#define PCI_COMMAND 0x04
#define PCI_BAR0 0x10

#define CONFIG_ADDRESS 0xCF8
#define CONFIG_DATA    0xCFC
#define NUM_BARS 6
#define NUM_BUS 256
#define MAX_DEVICES 20

// struct to "wrap" our devices
typedef struct {
    uint8_t bus;
    uint8_t slot;
    uint8_t function;
    uint16_t vendor;
    uint16_t device;
    uint8_t header;
    uint32_t bars[NUM_BARS];
} PciDevice;

// struct for to wrap a vendor device pair - used to for verificatoin 
typedef struct {
    uint16_t vendor;
    uint16_t device;
} VendorDevice;

// expected devices
extern VendorDevice expectedDevices[];

// discovered PCI devices
extern PciDevice pciDevices[NUM_BUS];
extern size_t pciDeviceCount;

// 32 bit read and writes
// bus - PCI bus number
// slot - device (slot) number
// function - function number.
// registerOffset - offset within the PCI config space
extern uint8_t pciConfigRead8(uint8_t bus, uint8_t slot, uint8_t function, uint8_t register_offset);

extern uint32_t pciConfigRead32(uint8_t bus, uint8_t slot, uint8_t function, uint8_t registerOffset);
extern void pciConfigWrite32(uint8_t bus, uint8_t slot, uint8_t function, uint8_t registerOffset, uint32_t value);

// returns a BAR value for the given device and BAR number
// For I/O BARs, it masks the lower 2 bits; for memory BARs, it extracts the base address
extern uint32_t getBar(PciDevice* device, uint32_t barNum);

// stores a device into the struct wrapper
extern void storePciDevice(uint8_t bus, uint8_t slot, uint8_t function);

// enumerates all PCI devices across all buses
extern void pciEnumDevices(void);

// finds and returns a pointer to the PCI device matching the vendor and device
extern PciDevice* findDeviceVD(uint16_t vendor, uint16_t device);

// verifies that all expected devices have been found
extern void verifyDevices(void);
