#include "pci.h"
#include "debug.h"

PciDevice pciDevices[NUM_BUS];
size_t pciDeviceCount = 0;

// reads from PCI config space
uint16_t pciConfigRead(uint8_t bus, uint8_t slot, uint8_t function, uint8_t register_offset) {
    uint32_t address = ((uint32_t)bus         << 16) |      // bus number 16-23
                       ((uint32_t)slot        << 11) |      // slot number 11-15
                       ((uint32_t)function    << 8)  |      // function 8-10
                       (register_offset & 0xFC)       |     // register offset
                       0x80000000;                          // enable bit in bit 31

    outl(CONFIG_ADDRESS, address);
    uint32_t data = inl(CONFIG_DATA);
    
    return (uint16_t)((data >> ((register_offset & 2) * 8)) & 0xFFFF);
}

// stores device
// TODO: Store BARs?
static void storePciDevice(uint8_t bus, uint8_t slot, uint8_t function) {
    if (pciDeviceCount >= NUM_BUS) {
        return;
    }

    pciDevices[pciDeviceCount].bus = bus;
    pciDevices[pciDeviceCount].slot = slot;
    pciDevices[pciDeviceCount].function = function;
    pciDevices[pciDeviceCount].vendor_id = pciConfigRead(bus, slot, function, 0);
    pciDevices[pciDeviceCount].device_id = pciConfigRead(bus, slot, function, 2);
    pciDevices[pciDeviceCount].header = (uint8_t)(pciConfigRead(bus, slot, function, 0xE) & 0xFF);

    Debug::printf("NEW DEVICE STORED: Bus %d, Slot %d, Function %d: Vendor 0x%x, Device ID = 0x%x\n", 
        pciDevices[pciDeviceCount].bus, pciDevices[pciDeviceCount].slot, pciDevices[pciDeviceCount].function,  
        pciDevices[pciDeviceCount].vendor_id, pciDevices[pciDeviceCount].device_id);
    
    pciDeviceCount++;
}

// enumerates all devices on all PCI buses
void pciEnumDevices() {
    for (uint8_t bus = 0; bus < 256 - 1; bus++) { // loop through all buses (0-254)
        for (uint8_t slot = 0; slot < 32; slot++) { // loop through all slots (0-31)
            uint16_t vendor = pciConfigRead(bus, slot, 0, 0);
            if (vendor != 0xFFFF) { // if vendor is valid
                storePciDevice(bus, slot, 0);
                // check header type from function 0 to see if device is multi function
                uint8_t header = pciConfigRead(bus, slot, 0, 0xE) & 0xFF;
                if (header & 0x80) {
                    for (uint8_t function = 1; function < 8; function++) {
                        uint16_t func_vendor = pciConfigRead(bus, slot, function, 0);
                        if (func_vendor == 0xFFFF) {
                            continue;
                        }
                        storePciDevice(bus, slot, function);
                    }
                }
            }
        }
    }
}

// find device(struct) based on vendor and device id
PciDevice* findDeviceVD(uint16_t vendor_id, uint16_t device_id) {
    for (size_t i = 0; i < pciDeviceCount; i++) {
        if ((pciDevices[i].vendor_id == vendor_id) && (pciDevices[i].device_id == device_id)) {
            return &pciDevices[i];
        }
    }
    return nullptr;
}