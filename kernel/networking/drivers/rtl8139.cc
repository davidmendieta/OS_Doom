// Adapted from https://github.com/szhou42/osdev/blob/master/src/kernel/drivers/rtl8139.c

#include "rtl8139.h"
#include "../../machine.h"
#include "../../heap.h"
#include "../../idt.h"
#include "../networking.h"
#include "../../physmem.h"
#include "../../smp.h"
#include "../protocols/parser.h"
#include "../socket/porttable.h"

#include "../arp/arp.h"

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
    uint32_t ret = pciConfigRead32(device->bus, device->slot, device->function, PCI_BAR0);


    bar_type = ret & 0x1;
    // Get io base or mem base by extracting the high 28/30 bits
    io_base = ret & (~0x3);
    mem_base = ret & (~0xf);
    Debug::printf("rtl8139 use %s access (base: %x)\n", (bar_type == 0)? "mem based":"port based", (bar_type != 0)?io_base:mem_base);

    // Set current TSAD
    tx_cur = 0;

    // Enable PCI Bus Mastering
    uint32_t pci_command_reg = pciConfigRead32(device->bus, device->slot, device->function, PCI_COMMAND); 
    if(!(pci_command_reg & (1 << 2))) {
        pci_command_reg |= (1 << 2);
        pciConfigWrite32(device->bus, device->slot, device->function, PCI_COMMAND, pci_command_reg);
    }

    // Send 0x00 to the CONFIG_1 register (0x52) to set the LWAKE + LWPTN to active high. this should essentially *power on* the device.
    outb(io_base + 0x52, 0x0);

    // Soft reset
    outb(io_base + 0x37, 0x10);
    while( (inb(io_base + 0x37) & 0x10) != 0) { }
    Debug::printf("val: %x\n", inb(io_base + 0x37));

    // Might be overkill, but we're 4096 aligning it - Allocate receive buffer
    void* raw_buf = malloc(8192 + 16 + 4096 + 1500); // overallocate
    uintptr_t raw_addr = (uintptr_t)raw_buf;
    uintptr_t aligned_addr = (raw_addr + 4095) & ~((uintptr_t)4095); // round up
    rx_buffer = (char*)aligned_addr;

    bzero(rx_buffer, 8192 + 16 + 1500);
    outl(io_base + 0x30, (uint32_t)rx_buffer); // We are telling the NIC where to write data it receives

    // (1 << 7) is the WRAP bit, 0xf is AB+AM+APM+AAP. This tells us what type of broadcasts we want to accept
    outl(io_base + 0x44, (1 << 1) | (1 << 3) | (1 << 7)); // APM, AB, WRAP
    uint32_t rcr_val = inl(io_base + 0x44);
    Debug::printf("RCR: %x\n", rcr_val);

    outw(io_base + 0x3E, 0xFFFF); // This resets interrupts

    outw(io_base + 0x3C, TOK | ROK); // We are telling the NIC that we want to trigger interrupts when we receive a transfer OK signal and a receive OK signal
    uint16_t mask = inw(io_base + 0x3C);
    Debug::printf("Interrupt Mask Register: %x\n", mask);
    
    // Sets the RE and TE bits high
    outb(io_base + 0x37, 0x0C); // We are allowing transfering and receiving on the NIC
    
    
    uint16_t irq_num = pciConfigRead32(device->bus, device->slot, device->function, PCI_INTERRUPT_LINE) & 0xFF; 



    // APIC Config
    IDT::interrupt(32 + irq_num, (uint32_t)RTL8139Handler_); // We're telling the CPU which function to call whenever it receives a signal at its IDT #32 + irq_num
    Debug::printf("Registered irq interrupt for rtl8139, irq num = %d\n", irq_num);

    #define IOAPIC_REGSEL  0xFEC00000
    #define IOAPIC_WINDOW  0xFEC00010
    #define REDIR_BASE     0x10

    uint32_t irq = irq_num;
    // We want to map the irq route #11 (or whatever irq is: this is on which line our NIC sends signals). From what I understand, REDIR_BASE is the start of the redirection table (mapping interrupt requests to an index in the IDT). There are 2 entries per IRQ (one low and one high), which explains why we add 2*irq.
    uint32_t redir_low = REDIR_BASE + 2 * irq;
    uint32_t redir_high = redir_low + 1;
    uint8_t vector = 32 + irq_num; // If we keep it this way for all devices, we shouldn't run into conflicts
    uint8_t dest_apic_id = 0; // This corresponds to which CPU is going to handle our NIC interrupts

    volatile uint32_t* sel = (volatile uint32_t*)IOAPIC_REGSEL;
    volatile uint32_t* win = (volatile uint32_t*)IOAPIC_WINDOW;

    // Set destination CPU
    *sel = redir_high;
    *win = ((uint32_t)dest_apic_id) << 24;

    // Unmask and set vector
    uint32_t flags = vector & 0xFF; // bits 0-7 = vector
    *sel = redir_low;
    *win = flags;

    
    read_mac_addr();

    uint8_t int_pin  = pciConfigRead8(device->bus, device->slot, device->function, 0x3D);
    uint8_t irq_line = pciConfigRead8(device->bus, device->slot, device->function, 0x3C);
    Debug::printf("PCI interrupt pin = %u, line = %u\n", int_pin, irq_line);
}

void* RTL8139::get_packet(uint32_t* packet_length) {
    uint16_t * t = (uint16_t*)(rx_buffer + current_packet_ptr);
    // Skip packet header, get packet length
    *packet_length = (uint32_t)*(t + 1);


    // print packet
    // Debug::printf("Packet length: %d\n", packet_length);
    // for (int i = 0; i < packet_length; i++) {
    //     Debug::printf("%02x ", *(t + 2 + i));
    // }

    // Skip, packet header and packet length, now t points to the packet data
    t = t + 2;
    //qemu_printf("Printing packet at addr 0x%x\n", (uint32_t)t);
    //xxd(t, packet_length);

    // Now, ethernet layer starts to handle the packet(be sure to make a copy of the packet, insteading of using the buffer)
    // and probabbly this should be done in a separate thread...
    void * packet = malloc(*packet_length);
    memcpy(packet, t, *packet_length);

    current_packet_ptr = (current_packet_ptr + *packet_length + 4 + 3) & RX_READ_POINTER_MASK;

    if(current_packet_ptr > RX_BUF_SIZE)
        current_packet_ptr -= RX_BUF_SIZE;

    outw(io_base + CAPR, current_packet_ptr - 0x10);

    return packet;
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

void RTL8139::get_mac_addr(char* src_mac_addr) {
    memcpy(src_mac_addr, mac_addr, 6);
}

void RTL8139::send_packet(const void* data, uint32_t len) {    
    // First, copy the data to a physically contiguous chunk of memory
    void* raw_buf = malloc(len + 4096); // overallocate
    uintptr_t raw_addr = (uintptr_t)raw_buf;
    uintptr_t aligned_addr = (raw_addr + 4095) & ~((uintptr_t)4095); // round up
    char* transfer_data = (char*)aligned_addr;

    memcpy((void*)transfer_data, data, len);
    Debug::printf("Sending Data\n");
    // Second, fill in physical address of data, and length
    outl(io_base + TSAD_array[tx_cur], (uint32_t)transfer_data);
    outl(io_base + TSD_array[tx_cur++], len);
    if(tx_cur > 3)
        tx_cur = 0;

    free(raw_buf);
}

extern "C" void RTL8139Handler() {
    //Debug::printf("RTL8139 interrupt was fired !!!! \n");
    uint16_t status = inw(network_interface->get_io_base() + 0x3e);

    
    //Debug::printf("STATUS: %d", status);
    
    if(status & TOK) {
        Debug::printf("Packet sent\n");
    }
    if (status & ROK) {
        //Debug::printf("Received packet\n");
        network_interface->receive();
    }
    
    SMP::eoi();
    outw(network_interface->get_io_base() + 0x3E, status); // clear interrupt

}
