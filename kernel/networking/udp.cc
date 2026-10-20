#include "udp.h"
#include "drivers/rtl8139.h"
#include "../blocking_lock.h"
#include "../threads.h"
#include "../pit.h"

namespace {
BlockingLock lock;
RTL8139* nic;
uint32_t address;
os_net_stats counters{};
struct Endpoint {
    const void* owner;
    unsigned port, head, count;
    os_datagram queue[32];
} endpoints[8]{};
struct Neighbor { uint32_t ip, probe; uint8_t mac[6]; bool known; } neighbors[16]{};
unsigned next_neighbor, ephemeral = 49152;
uint8_t frame[1514];
bool equal_mac(const uint8_t* a, const uint8_t* b) {
    for (unsigned i = 0; i < 6; ++i) if (a[i] != b[i]) return false;
    return true;
}
unsigned get16(const uint8_t* p) { return (p[0] << 8) | p[1]; }
uint32_t get32(const uint8_t* p) { return (uint32_t(get16(p)) << 16) | get16(p + 2); }
void put16(uint8_t* p, unsigned v) { p[0] = v >> 8; p[1] = v; }
void put32(uint8_t* p, uint32_t v) { put16(p, v >> 16); put16(p + 2, v); }
uint32_t sum(const uint8_t* p, unsigned n, uint32_t total = 0) {
    while (n > 1) { total += get16(p); p += 2; n -= 2; }
    if (n) total += p[0] << 8;
    return total;
}
unsigned checksum(uint32_t value) { while (value >> 16) value = (value & 65535) + (value >> 16); return (~value) & 65535; }
Neighbor& neighbor(uint32_t ip) {
    for (auto& n : neighbors) if (n.ip == ip) return n;
    auto& n = neighbors[next_neighbor++ % 16];
    n = {}; n.ip = ip; return n;
}
void learn(uint32_t ip, const uint8_t* mac) {
    if (!ip || ip == address || (mac[0] & 1)) return;
    auto& n = neighbor(ip); memcpy(n.mac, mac, 6); n.known = true;
}
void arp(uint32_t ip, const uint8_t* mac, unsigned operation) {
    uint8_t packet[42]{};
    memcpy(packet, mac, 6); memcpy(packet + 6, nic->mac_addr, 6);
    put16(packet + 12, 0x806); put16(packet + 14, 1); put16(packet + 16, 0x800);
    packet[18] = 6; packet[19] = 4; put16(packet + 20, operation);
    memcpy(packet + 22, nic->mac_addr, 6); put32(packet + 28, address);
    if (operation == 2) memcpy(packet + 32, mac, 6);
    put32(packet + 38, ip);
    if (nic->transmit(packet, sizeof(packet))) {
        if (operation == 1) ++counters.arp_requests; else ++counters.arp_replies;
    }
}
void deliver(uint32_t ip, unsigned source, unsigned port, const uint8_t* data, unsigned length) {
    for (auto& e : endpoints) if (e.owner && e.port == port) {
        if (e.count == 32) { ++counters.dropped; return; }
        auto& p = e.queue[(e.head + e.count++) % 32];
        p = {}; p.ip = ip; p.port = source; p.length = length; memcpy(p.data, data, length);
        ++counters.rx; return;
    }
    ++counters.dropped;
}
void poll() {
    if (!nic || !nic->ready || !address) return;
    for (unsigned budget = 0; budget < 64; ++budget) {
        int n = nic->receive(frame, sizeof(frame));
        if (!n) break;
        if (n < 14) { ++counters.dropped; continue; }
        if (get16(frame + 12) == 0x806) {
            if (n < 42 || get16(frame + 14) != 1 || get16(frame + 16) != 0x800 || frame[18] != 6 || frame[19] != 4 ||
                (get16(frame + 20) != 1 && get16(frame + 20) != 2) || get32(frame + 38) != address ||
                !equal_mac(frame + 6, frame + 22)) { ++counters.dropped; continue; }
            learn(get32(frame + 28), frame + 22);
            if (get16(frame + 20) == 1) arp(get32(frame + 28), frame + 22, 2);
            continue;
        }
        if (get16(frame + 12) != 0x800 || n < 42) { ++counters.dropped; continue; }
        uint8_t* ip = frame + 14;
        unsigned ihl = (ip[0] & 15) * 4, length = get16(ip + 2);
        if ((ip[0] >> 4) != 4 || ihl < 20 || length < ihl + 8 || length > unsigned(n - 14) ||
            get32(ip + 16) != address || ip[9] != 17 || !ip[8] || (get16(ip + 6) & 0xbfff) ||
            checksum(sum(ip, ihl)) != 0) { ++counters.dropped; continue; }
        uint8_t* udp = ip + ihl;
        unsigned size = get16(udp + 4);
        if (size < 8 || size != length - ihl || size - 8 > OS_UDP_MAX || !get16(udp + 2) ||
            (get16(udp + 6) && checksum(sum(udp, size, sum(ip + 12, 8) + 17 + size)))) {
            ++counters.dropped; continue;
        }
        learn(get32(ip + 12), frame + 6);
        deliver(get32(ip + 12), get16(udp), get16(udp + 2), udp + 8, size - 8);
    }
}
const void* owner() { return impl::threads::state.current()->pcb.operator->(); }
Endpoint* endpoint(int fd) {
    if (fd < 0 || fd >= 8 || endpoints[fd].owner != owner()) return nullptr;
    return &endpoints[fd];
}
}
void NativeNet::init() { nic = new RTL8139(); }
int NativeNet::configure(uint32_t ip) {
    LockGuard guard{lock};
    if (!nic || !nic->ready || (ip >> 24) == 0 || (ip >> 24) >= 224 || (ip & 255) == 255 || !(ip & 255)) return -1;
    if (address && address != ip) for (auto& e : endpoints) if (e.owner) return -1;
    if (address != ip) for (auto& n : neighbors) n = {};
    address = ip;
    Debug::printf("*** net: IPv4 %u.%u.%u.%u\n", ip >> 24, (ip >> 16) & 255, (ip >> 8) & 255, ip & 255);
    return 0;
}
int NativeNet::open(unsigned port) {
    LockGuard guard{lock};
    if (!address || port > 65535) return -1;
    if (!port) {
        for (unsigned tries = 0; tries < 16384; ++tries) {
            port = ephemeral++; if (ephemeral > 65535) ephemeral = 49152;
            bool taken = false; for (auto& e : endpoints) if (e.owner && e.port == port) taken = true;
            if (!taken) break;
        }
    }
    for (auto& e : endpoints) if (e.owner && e.port == port) return -1;
    for (unsigned i = 0; i < 8; ++i) if (!endpoints[i].owner) {
        auto& e = endpoints[i]; e.owner = owner(); e.port = port; e.head = e.count = 0; return i;
    }
    return -1;
}
int NativeNet::close(int fd) {
    LockGuard guard{lock}; auto* e = endpoint(fd); if (!e) return -1;
    e->owner = nullptr; e->count = 0; return 0;
}
int NativeNet::receive(int fd, os_datagram* packet) {
    LockGuard guard{lock}; auto* e = endpoint(fd); if (!e) return -1;
    poll(); if (!e->count) return 0;
    *packet = e->queue[e->head]; e->head = (e->head + 1) % 32; --e->count; return 1;
}
int NativeNet::send(int fd, const os_datagram* packet) {
    LockGuard guard{lock}; auto* e = endpoint(fd);
    if (!e || packet->length > OS_UDP_MAX || !packet->port || !packet->ip || packet->ip >= 0xe0000000) return -1;
    poll();
    if (packet->ip == address) {
        deliver(address, e->port, packet->port, packet->data, packet->length); ++counters.tx; return packet->length;
    }
    auto& peer = neighbor(packet->ip);
    if (!peer.known) {
        uint32_t now = Pit::milliseconds();
        if (!peer.probe || uint32_t(now - peer.probe) >= 200) {
            static const uint8_t broadcast[6] = {255,255,255,255,255,255};
            arp(packet->ip, broadcast, 1); peer.probe = now ? now : 1;
        }
        return -2;
    }
    uint8_t output[1514]{};
    memcpy(output, peer.mac, 6); memcpy(output + 6, nic->mac_addr, 6); put16(output + 12, 0x800);
    uint8_t* ip = output + 14; ip[0] = 0x45; put16(ip + 2, packet->length + 28);
    put16(ip + 6, 0x4000); ip[8] = 64; ip[9] = 17; put32(ip + 12, address); put32(ip + 16, packet->ip);
    put16(ip + 10, checksum(sum(ip, 20)));
    uint8_t* udp = ip + 20; put16(udp, e->port); put16(udp + 2, packet->port); put16(udp + 4, packet->length + 8);
    memcpy(udp + 8, packet->data, packet->length);
    unsigned check = checksum(sum(udp, packet->length + 8, sum(ip + 12, 8) + 17 + packet->length + 8));
    put16(udp + 6, check ? check : 65535);
    if (!nic->transmit(output, packet->length + 42)) return -2;
    ++counters.tx; return packet->length;
}
int NativeNet::stats(os_net_stats* out) { LockGuard guard{lock}; poll(); *out = counters; return 0; }
