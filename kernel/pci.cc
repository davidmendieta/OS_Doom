#include "pci.h"
#include "debug.h"

PciDevice pciDevices[NUM_BUS];
size_t pciDeviceCount = 0;

// the devices we expect to have found after our scan
VendorDevice expectedDevices[] = {
    { 0x8086, 0x1237 }, // Intel 82441FX Host Bridge (440FX chipset) - communication between the CPU and system peripherals
    { 0x8086, 0x7000 }, // Intel 82371SB PIIX3 ISA Bridge - connects the CPU with legacy ISA devices in the Intel 440FX chipset
    { 0x8086, 0x7010 }, // Intel 82371SB PIIX3 IDE Controller - handles IDE storage devices in the Intel 440FX chipset
    { 0x8086, 0x7113 }, // Intel 82371 PIIX4 ACPI Controller - manages power configuration and ACPI functions
    { 0x1234, 0x1111 }, // Bochs/QEMU Virtual VGA Adapter - emulates a VGA compatible graphics device for basic virtual machine display
    { 0x8086, 0x100e }, // Intel 82540EM Gigabit Ethernet Controller (e1000) - provides gigabit network connectivity
};

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

// write to PCI config space
void pciConfigWrite(uint8_t bus, uint8_t slot, uint8_t function, uint8_t register_offset, uint16_t value) {
    uint32_t address = ((uint32_t)bus         << 16) |      // bus number 16-23
                       ((uint32_t)slot        << 11) |      // slot number 11-15
                       ((uint32_t)function    << 8)  |      // function 8-10
                       (register_offset & 0xFC)       |     // register offset
                       0x80000000;                          // enable bit in bit 31

    outl(CONFIG_ADDRESS, address);
    uint32_t old_data = inl(CONFIG_DATA);
    uint32_t new_data = (old_data & ~(0xFFFF << ((register_offset & 2) * 8))) | ((uint32_t)value << ((register_offset & 2) * 8));

    outl(CONFIG_DATA, new_data);
}


// stores device
static void storePciDevice(uint8_t bus, uint8_t slot, uint8_t function) {
    if (pciDeviceCount >= NUM_BUS) {
        return;
    }

    PciDevice* device = &pciDevices[pciDeviceCount];
    device->bus = bus;
    device->slot = slot;
    device->function = function;
    device->vendor_id = pciConfigRead(bus, slot, function, 0);
    device->device_id = pciConfigRead(bus, slot, function, 2);
    device->header = (uint8_t)(pciConfigRead(bus, slot, function, 0xE) & 0xFF);

    // read bars
    int max_bars = (device->header & 0x7F) == 1 ? 2 : 6;
    for (int i = 0; i < max_bars; i++) {
        device->bars[i] = pciConfigRead(bus, slot, function, 0x10 + (i * 4));
    }
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

    verifyDevices();
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

// verify we find all of our expected devices
void verifyDevices() {
    for (size_t i = 0; i < pciDeviceCount; i++) {
        PciDevice* device = &pciDevices[i];
        bool found = false;

        for (int j = 0; j < MAX_DEVICES; j++) {
            if (expectedDevices[j].vendor == device->vendor_id &&
                expectedDevices[j].device == device->device_id) {
                found = true;
                break;
            }
        }

        if (found) {
            Debug::printf(" device found: Vendor 0x%x, Device 0x%x\n",
                device->vendor_id, device->device_id);
        } else {
            Debug::panic("expected device not found: Vendor 0x%x, Device 0x%x\n", device->vendor_id, device->device_id);
        }
    }

    Debug::printf("** successfully found all expected devices **\n");
}
