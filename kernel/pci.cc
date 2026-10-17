#include "pci.h"
#include "debug.h"

PciDevice pciDevices[NUM_BUS];
size_t pciDeviceCount = 0;

uint8_t pciConfigRead8(uint8_t bus, uint8_t slot, uint8_t function, uint8_t register_offset) {
    uint32_t address = (1U << 31)                  // enable bit
                     | ((uint32_t)bus << 16)
                     | ((uint32_t)slot << 11)
                     | ((uint32_t)function << 8)
                     | (register_offset & 0xFC);   // aligned offset

    outl(CONFIG_ADDRESS, address);
    uint32_t data = inl(CONFIG_DATA);

    uint8_t shift = (register_offset & 0x3) * 8;
    return (uint8_t)((data >> shift) & 0xFF);
}


// the devices we expect to have found after our scan
VendorDevice expectedDevices[] = {
    { 0x8086, 0x1237 }, // Intel 82441FX Host Bridge (440FX chipset) - communication between the CPU and system peripherals
    { 0x8086, 0x7000 }, // Intel 82371SB PIIX3 ISA Bridge - connects the CPU with legacy ISA devices in the Intel 440FX chipset
    { 0x8086, 0x7010 }, // Intel 82371SB PIIX3 IDE Controller - handles IDE storage devices in the Intel 440FX chipset
    { 0x8086, 0x7113 }, // Intel 82371 PIIX4 ACPI Controller - manages power configuration and ACPI functions
    { 0x1234, 0x1111 }, // Bochs/QEMU Virtual VGA Adapter - emulates a VGA compatible graphics device for basic virtual machine display
    { 0x8086, 0x100e }, // Intel 82540EM Gigabit Ethernet Controller (e1000) - provides gigabit network connectivity
    { 0x8086, 0x2415 }, // Intel 82801AA/ICH AC’97 Audio Controller
};

// reads from PCI config space - 32 bits
uint32_t pciConfigRead32(uint8_t bus, uint8_t slot, uint8_t function, uint8_t registerOffset) {
    uint32_t address = ((uint32_t)bus         << 16) |      // bus number 16-23
                       ((uint32_t)slot        << 11) |      // slot number 11-15
                       ((uint32_t)function    << 8)  |      // function 8-10
                       (registerOffset & 0xFC)       |      // register offset
                       0x80000000;                          // enable bit in bit 31

    outl(CONFIG_ADDRESS, address);  
    return inl(CONFIG_DATA);
    
}

// write to PCI config space - 32 bits
void pciConfigWrite32(uint8_t bus, uint8_t slot, uint8_t function, uint8_t registerOffset, uint32_t value) {
    uint32_t address = ((uint32_t)bus      << 16) |   // bus number 16-23
                       ((uint32_t)slot     << 11) |   // slot number 11-15
                       ((uint32_t)function << 8)  |   // function 8-10
                       (registerOffset & 0xFC)    |   // register offset (aligned to 4 bytes)
                       0x80000000;                    // enable bit in bit 31

    outl(CONFIG_ADDRESS, address);
    outl(CONFIG_DATA, value);
}

// gets a specific BAR from a device
uint32_t getBar(PciDevice* device, uint32_t barNum) {
    if (barNum >= NUM_BARS) {
        Debug::panic("invalid BAR number %d: Vendor 0x%x, Device 0x%x\n", barNum, device->vendor, device->device);
    }

    uint32_t bar = device->bars[barNum];

    // no addy
    if (bar == 0) {
        return 0;
    }

    if (bar & 0x1) { // I/O BAR
        return (bar & 0xFFFFFFFC);
    } else { // Memory BAR - check 32 bit or 64 bit
        uint32_t type = (bar >> 1) & 0x3;
        if (type == 0x0) { // 32 bit
            return (bar & 0xFFFFFFF0);
        } else if (type == 0x2) { // 64 bit
            uint32_t barHigh = device->bars[barNum + 1];
            // since were on a 32 bit system, we only allow 64 bit BAR if the high part is zero
            if (barHigh != 0) {
                Debug::panic("64 bit BAR[%d] with high bits set: 0x%x%08x\n", barNum, barHigh, bar & 0xFFFFFFF0);
            }
            return (bar & 0xFFFFFFF0);
        } else {
            Debug::panic("invalid BAR type %d at BAR[%d]: Vendor 0x%x, Device 0x%x\n", type, barNum, device->vendor, device->device);
        }
    }
    return 0;
}

// stores device
void storePciDevice(uint8_t bus, uint8_t slot, uint8_t function) {
    if (pciDeviceCount >= NUM_BUS) {
        return;
    }

    PciDevice* device = &pciDevices[pciDeviceCount];
    device->bus = bus;
    device->slot = slot;
    device->function = function;

    // read 32 bits at offset 0 which contain vendor and device
    uint32_t vendor_and_device = pciConfigRead32(bus, slot, function, 0);
    device->vendor = vendor_and_device & 0xFFFF;  // vendor ID in lower 16 bits
    device->device= (vendor_and_device >> 16) & 0xFFFF; // device ID in upper 16 bits

    // read 32 bits at offset 0x0C which contains header
    uint32_t header = pciConfigRead32(bus, slot, function, 0x0C);
    device->header = (header >> 16) & 0xFF;

    // read BARs starting at offset 0x10
    int maxBARS = ((device->header & 0x7F) == 1) ? 2 : 6; // 2 BARs for header type 1 - otherwise 6 
    for (int i = 0; i < maxBARS; i++) {
        device->bars[i] = pciConfigRead32(bus, slot, function, 0x10 + (i * 4));
    }
    pciDeviceCount++;
}

// enumerates all devices on all PCI buses
void pciEnumDevices() {
    for (uint8_t bus = 0; bus < 255; bus++) { // loop through all buses (0-254)
        for (uint8_t slot = 0; slot < 32; slot++) { // loop through all slots (0-31)
            uint32_t vendorData = pciConfigRead32(bus, slot, 0, 0);
            uint16_t vendor = vendorData & 0xFFFF;
            if (vendor != 0xFFFF) { // if vendor is valid
                storePciDevice(bus, slot, 0);
                // check header type from function 0 to see if device is multi function
                uint32_t headerData = pciConfigRead32(bus, slot, 0, 0x0C);
                uint8_t header = (headerData >> 16) & 0xFF;
                if (header & 0x80) { // device is multifunction
                    for (uint8_t function = 1; function < 8; function++) {
                        uint32_t funcData = pciConfigRead32(bus, slot, function, 0);
                        uint16_t funcVendor = funcData & 0xFFFF;
                        if (funcVendor == 0xFFFF) {
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
    Debug::printf("Finding device id\n");
    for (size_t i = 0; i < pciDeviceCount; i++) {
        if ((pciDevices[i].vendor == vendor_id) && 
            (pciDevices[i].device == device_id)) {
            return &pciDevices[i];
        }
    }
	Debug::printf("didn't find device id\n");
    return nullptr;
}

// verify we find all of our expected devices
void verifyDevices() {
    for (size_t i = 0; i <  sizeof(expectedDevices)/sizeof(expectedDevices[0]); i++) {
        VendorDevice* expected = &expectedDevices[i];
        PciDevice* device = findDeviceVD(expected->vendor, expected->device);

        if (device != nullptr) {
            Debug::printf("| Expected Device Found: Vendor 0x%x, Device 0x%x\n", expected->vendor, expected->device);

            int max_bars = (device->header & 0x7F) == 1 ? 2 : 6;
            for (int b = 0; b < max_bars; b++) {
                if (device->bars[b] != 0) {
                    Debug::printf("    | BAR[%d]: 0x%08x\n", b, device->bars[b]);
                }
            }
        } else {
            Debug::panic("| Expected Device Not Found: Vendor 0x%x, Device 0x%x\n", expected->vendor, expected->device);
        }
    }

    Debug::printf("- FOUND ALL EXPECTED DEVICES - \n\n");
}
