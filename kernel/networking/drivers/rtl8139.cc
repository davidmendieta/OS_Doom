// RTL8139 legacy DMA mode. Polling keeps allocation and socket work out of IRQs.
#include "rtl8139.h"
#include "../../machine.h"
#include "../../heap.h"
#include "../../smp.h"

namespace {
alignas(4096) uint8_t receive_dma[RX_BUF_SIZE + 16 + 2048];
alignas(4096) uint8_t transmit_dma[4][2048];
}
RTL8139::RTL8139() {
    device = findDeviceVD(RTL8139_VENDOR_ID, RTL8139_DEVICE_ID);
    if (!device) return;
    uint32_t bar = pciConfigRead32(device->bus, device->slot, device->function, PCI_BAR0);
    if (!(bar & 1) || (bar & ~3u) > 65535) return;
    io_base = bar & ~3u;
    bar_type = 1;
    uint32_t command = pciConfigRead32(device->bus, device->slot, device->function, PCI_COMMAND) & 65535;
    pciConfigWrite32(device->bus, device->slot, device->function, PCI_COMMAND, command | 5);
    outb(io_base + Config1, 0);
    outb(io_base + ChipCmd, 0x10);
    unsigned tries = 100000;
    while ((inb(io_base + ChipCmd) & 0x10) && --tries) {}
    if (!tries) return;
    rx_buffer = (char*)receive_dma;
    tx_cur = 0;
    outw(io_base + IntrMask, 0);
    outw(io_base + IntrStatus, 0xffff);
    outl(io_base + RxBuf, (uint32_t)receive_dma);
    outb(io_base + ChipCmd, 0x0c);
    outl(io_base + RxConfig, (1 << 7) | 0x0a); // wrap spill area, own MAC and broadcast
    outl(io_base + TxConfig, (3 << 24) | (6 << 8));
    read_mac_addr();
    ready = true;
    Debug::printf("*** net: RTL8139 polling DMA ready\n");
}
void RTL8139::read_mac_addr() {
    for (unsigned i = 0; i < 6; ++i) mac_addr[i] = inb(io_base + i);
}
void RTL8139::get_mac_addr(char* out) { memcpy(out, mac_addr, 6); }
bool RTL8139::transmit(const void* data, uint32_t length) {
    if (!ready || length < 14 || length > 1514) return false;
    // OWN is returned by the NIC only after this descriptor's DMA completes.
    if (!(inl(io_base + TxStatus0 + tx_cur * 4) & 0x2000)) return false;
    unsigned padded = length < 60 ? 60 : length;
    bzero(transmit_dma[tx_cur], padded);
    memcpy(transmit_dma[tx_cur], data, length);
    asm volatile("" : : : "memory");
    outl(io_base + TxAddr0 + tx_cur * 4, (uint32_t)transmit_dma[tx_cur]);
    outl(io_base + TxStatus0 + tx_cur * 4, padded);
    tx_cur = (tx_cur + 1) % 4;
    return true;
}
int RTL8139::receive(void* data, uint32_t capacity) {
    if (!ready) return 0;
    uint16_t interrupts = inw(io_base + IntrStatus);
    if (interrupts & 0x50) { // recover after a bounded ring overrun
        outb(io_base + ChipCmd, 4);
        current_packet_ptr = 0;
        outl(io_base + RxBuf, (uint32_t)receive_dma);
        outb(io_base + ChipCmd, 0x0c);
        outl(io_base + RxConfig, (1 << 7) | 0x0a);
        outw(io_base + IntrStatus, interrupts);
        return -1;
    }
    if (inb(io_base + ChipCmd) & 1) return 0;
    volatile uint16_t* header = (volatile uint16_t*)(receive_dma + current_packet_ptr);
    unsigned status = header[0], size = header[1]; // includes Ethernet CRC
    if (size == 0xfff0) return 0; // DMA still in progress
    if (size < 18 || size > 1518) {
        outb(io_base + ChipCmd, 4);
        current_packet_ptr = 0;
        outl(io_base + RxBuf, (uint32_t)receive_dma);
        outb(io_base + ChipCmd, 0x0c);
        outw(io_base + IntrStatus, 0xffff);
        return -1;
    }
    int copied = -1;
    if ((status & 1) && !(status & 0x3e) && size - 4 <= capacity) {
        memcpy(data, receive_dma + current_packet_ptr + 4, size - 4);
        copied = size - 4;
    }
    current_packet_ptr = (current_packet_ptr + size + 7) & ~3u;
    current_packet_ptr %= RX_BUF_SIZE;
    outw(io_base + CAPR, uint16_t(current_packet_ptr - 16));
    outw(io_base + IntrStatus, interrupts);
    return copied;
}
// Legacy adapter; the integrated stack uses receive() with bounded storage.
void* RTL8139::get_packet(uint32_t* length) {
    void* data = malloc(1514);
    int n = receive(data, 1514);
    if (n <= 0) { free(data); *length = 0; return nullptr; }
    *length = n;
    return data;
}
extern "C" void RTL8139Handler() { SMP::eoi(); } // NIC interrupts are masked
