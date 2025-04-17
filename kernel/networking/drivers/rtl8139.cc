// Adapted from https://github.com/szhou42/osdev/blob/master/src/kernel/drivers/rtl8139.c

#include "rtl8139.h"
#include "../../machine.h"
#include "../../heap.h"
#include "../../idt.h"
#include "../networking.h"
#include "../../physmem.h"
#include "../../pci.h"

PciDevice* device;

uint32_t current_packet_ptr;

// Four TXAD register, you must use a different one to send packet each time(for example, use the first one, second... fourth and back to the first)
uint8_t TSAD_array[4] = {0x20, 0x24, 0x28, 0x2C};
uint8_t TSD_array[4] = {0x10, 0x14, 0x18, 0x1C};

alignas(16) uint8_t tx_buf[4][60];


/*
 * Initialize the rtl8139 card driver
 * */
RTL8139::RTL8139(){
    // First get the network device using PCI
    device = findDeviceVD(RTL8139_VENDOR_ID, RTL8139_DEVICE_ID);
    uint32_t ret = pciConfigRead(device->bus, device->slot, device->function, PCI_BAR0);


    bar_type = ret & 0x1;
    // Get io base or mem base by extracting the high 28/30 bits
    io_base = ret & (~0x3);
    mem_base = ret & (~0xf);
    Debug::printf("rtl8139 use %s access (base: %x)\n", (bar_type == 0)? "mem based":"port based", (bar_type != 0)?io_base:mem_base);

    // Set current TSAD
    tx_cur = 0;

    // Enable PCI Bus Mastering
    uint32_t pci_command_reg = pciConfigRead(device->bus, device->slot, device->function, PCI_COMMAND); 
    if(!(pci_command_reg & (1 << 2))) {
        pci_command_reg |= (1 << 2);
        pciConfigWrite(device->bus, device->slot, device->function, PCI_COMMAND, pci_command_reg);
    }

    // Send 0x00 to the CONFIG_1 register (0x52) to set the LWAKE + LWPTN to active high. this should essentially *power on* the device.
    outb(io_base + 0x52, 0x0);

    // Soft reset
    outb(io_base + 0x37, 0x10);
    while( (inb(io_base + 0x37) & 0x10) != 0) { }
    
    Debug::printf("val: %x\n", inb(io_base + 0x37));
    // Allocate receive buffer

    // Might be overkill
    void* raw_buf = malloc(8192 + 16 + 4096 + 1500); // overallocate
    uintptr_t raw_addr = (uintptr_t)raw_buf;
    uintptr_t aligned_addr = (raw_addr + 4095) & ~((uintptr_t)4095); // round up
    rx_buffer = (char*)aligned_addr;

    bzero(rx_buffer, 8192 + 16 + 1500);
    outl(io_base + 0x30, (uint32_t)rx_buffer);

    // (1 << 7) is the WRAP bit, 0xf is AB+AM+APM+AAP
    outl(io_base + 0x44, 0xf | (1 << 7));
    uint32_t rcr_val = inl(io_base + 0x44);
    Debug::printf("RCR: %x\n", rcr_val);

    outw(io_base + 0x3E, 0xFFFF);

    outw(io_base + 0x3C, TOK | ROK);
    uint16_t mask = inw(io_base + 0x3C);
    Debug::printf("Interrupt Mask Register: %x\n", mask);
    
    // Sets the RE and TE bits high
    outb(io_base + 0x37, 0x0C);
    
    
    

    Debug::printf("IO BASE: %d\n", io_base);
    
    // Sets the TOK and ROK bits high)    
    uint8_t cmd = inb(io_base + 0x37);
    Debug::printf("Command Register: %x\n", cmd);
    
    
    
    // Register and enable network interrupts
    uint16_t irq_num = pciConfigRead(device->bus, device->slot, device->function, PCI_INTERRUPT_LINE) & 0xFF; 
    IDT::interrupt(32 + irq_num, (uint32_t)RTL8139Handler_);
    Debug::printf("Registered irq interrupt for rtl8139, irq num = %d\n", irq_num);
    
    read_mac_addr();

}

void RTL8139::receive_packet() {
    uint16_t * t = (uint16_t*)(rx_buffer + current_packet_ptr);
    // Skip packet header, get packet length
    uint16_t packet_length = *(t + 1);

    // Skip, packet header and packet length, now t points to the packet data
    t = t + 2;
    //qemu_printf("Printing packet at addr 0x%x\n", (uint32_t)t);
    //xxd(t, packet_length);

    // Now, ethernet layer starts to handle the packet(be sure to make a copy of the packet, insteading of using the buffer)
    // and probabbly this should be done in a separate thread...
    void * packet = malloc(packet_length);
    memcpy(packet, t, packet_length);

    // ethernet_handle_packet(packet, packet_length);

    current_packet_ptr = (current_packet_ptr + packet_length + 4 + 3) & RX_READ_POINTER_MASK;

    if(current_packet_ptr > RX_BUF_SIZE)
        current_packet_ptr -= RX_BUF_SIZE;

    outw(io_base + CAPR, current_packet_ptr - 0x10);
}

void RTL8139::read_mac_addr() {
    uint32_t mac_part1 = inl(io_base + 0x00);
    uint16_t mac_part2 = inw(io_base + 0x04);
    mac_addr[0] = mac_part1 >> 0;
    mac_addr[1] = mac_part1 >> 8;
    mac_addr[2] = mac_part1 >> 16;
    mac_addr[3] = mac_part1 >> 24;

    mac_addr[4] = mac_part2 >> 0;
    mac_addr[5] = mac_part2 >> 8;
    Debug::printf("MAC Address: %01x:%01x:%01x:%01x:%01x:%01x\n", mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
}

void RTL8139::get_mac_addr(uint8_t* src_mac_addr) {
    memcpy(src_mac_addr, mac_addr, 6);
}

void RTL8139::send_packet(void * data, uint32_t len) {
    // First, copy the data to a physically contiguous chunk of memory
    void* transfer_data = malloc(len); //! WAS ORIGINALLY ALIGNED - MIGHT CAUSE ISSUES
    memcpy(transfer_data, data, len);
    Debug::printf("Sending Data\n");
    // Second, fill in physical address of data, and length
    outl(io_base + TSAD_array[tx_cur], (uint32_t)transfer_data);
    outl(io_base + TSD_array[tx_cur++], len);
    if(tx_cur > 3)
        tx_cur = 0;
}

extern "C" void RTL8139Handler() {
    Debug::printf("RTL8139 interrupt was fired !!!! \n");
    uint16_t status = inw(rtl8139->io_base + 0x3e);

    if(status & TOK) {
        Debug::printf("Packet sent\n");
    }
    if (status & ROK) {
        Debug::printf("Received packet\n");
        rtl8139->receive_packet();
    }

    outw(rtl8139->io_base + 0x3E, 0x5); // clear interrupt
}