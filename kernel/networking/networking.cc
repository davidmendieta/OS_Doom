#include "networking.h"
#include "parser.h"
// #include <unordered_map>
// #include <cstdint>
// #include <smp.h>

// // UDP port‑handler function signature
// using UdpHandler = void (*)(const uint8_t* data,
//     size_t       len,
//     uint32_t     srcIp,
//     uint16_t     srcPort);

// // registry mapping destination port → handler
// static std::unordered_map<uint16_t, UdpHandler> udpHandlers;

// // register a callback for incoming packets on `port`
// void registerUdpHandler(uint16_t port, UdpHandler h) {
//     udpHandlers[port] = h;
// }

// // entry point from the RTL8139 driver for each received Ethernet frame
// void ethernet_handle_packet(void* packet, size_t len) {
//     size_t   payloadLen;
//     uint32_t srcIp;
//     uint16_t srcPort, dstPort;

//     // peel off Ethernet/IPv4/UDP, verify checksums, extract payload + metadata
//     void* data = Parser::receive(
//         packet, len,
//         &payloadLen,
//         &srcIp,
//         &srcPort,
//         &dstPort
//     );
//     if (!data) {
//         free(packet);       
//         return;
//     }   

//     // dispatch to the registered handler (if any)
//     auto it = udpHandlers.find(dstPort);
//     if (it != udpHandlers.end()) it->second((const uint8_t*)data, payloadLen, srcIp, srcPort);

//     free(data);
//     free(packet);
// }


StrongPtr<RTL8139> rtl8139;

void networkingMain(void) {
    // --------------- CLIENT CODE ----------------
    rtl8139 = StrongPtr<RTL8139>::make();

    uint8_t test_packet[60] __attribute__((aligned(16))) = {
        0xff,0xff,0xff,0xff,0xff,0xff, // broadcast dest MAC
        0x12,0x34,0x56,0x78,0x9a,0xbc, // source MAC
        0x08,0x00,                     // Ethertype: IPv4
        'h','e','l','l','o'
    };
    rtl8139->send_packet(test_packet, 60);

    // --------------- HOST CODE -----------------
    // The following works but we should do it with interrupts instead
    while (1) {
    }
}