// HOST
/*
#include "networking.h"

StrongPtr<NetworkInterface> network_interface;

void networkingMain(void) {
    network_interface = StrongPtr<NetworkInterface>::make(19216812); // My IP address

    // network_interface->send_ip_udp("Hello", 5, 19216812, 1234, 19216812, 4321, nullptr);
    while(1){
    };

}
*/

// CLIENT
/*
#include "networking.h"
#include "../pit.h"
#include "../threads.h"
#include "../machine.h"
#include "../heap.h"

static constexpr uint32_t DEST_IP = 19216812;
static constexpr uint16_t DEST_PORT = 4321;
static constexpr uint32_t SRC_IP = 19216810;
static constexpr uint16_t SRC_PORT  = 1234;

static const int N = 1000;

StrongPtr<NetworkInterface> network_interface;

void fill(char* buf, char value, size_t sz) {
    for (size_t i = 0; i < sz; ++i) {
        buf[i] = value;
    }
}

void networkingMain(void) {
    network_interface = StrongPtr<NetworkInterface>::make(SRC_IP); // My IP address
    network_interface->send_arp_request(DEST_IP);

    // ─── Benchmark 1: throughput vs. payload size ───
    Debug::printf("\n\n ----- starting benchmark 1\n\n");
    //const uint32_t sizes[] = { 32, 128, 512, 1024 };
    const uint32_t sizes[] = {256, 512, 1024, 2048};
    for (auto sz : sizes) {
        auto buf = (char*) malloc(sz);
        fill(buf, 'A', sz);

        //Debug::printf("\n\n ----- starting starting for loop\n\n");
        uint32_t start = Pit::jiffies;
        for (int i = 0; i < N; i++) {
            // send_ip_udp(payload, payload_len, src_ip, src_port, dest_ip, dest_port, out_len)
            //Debug::printf("\n\n------------- sending packet\n\n");
            network_interface->send_ip_udp(buf, sz,
                SRC_IP, SRC_PORT,
                DEST_IP, DEST_PORT,
                nullptr);
        }
        uint32_t elapsed = Pit::jiffies - start;
        Debug::printf("[B1] %4u-byte x %4d sends = %u jiffies\n", sz, N, elapsed);
        free(buf);
    }

}
*/
