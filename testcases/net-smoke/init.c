#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <os/net.h>
#include <os/runtime.h>

static void send_packet(int fd, os_datagram *p) {
    unsigned start = os_milliseconds();
    int n;
    while ((n = os_udp_send(fd, p)) == -2) {
        assert(os_milliseconds() - start < 10000);
        os_yield();
    }
    assert(n == p->length);
}
int main(void) {
    unsigned role, ip, server;
    os_datagram p = {0}, reply;
    setvbuf(stdout, NULL, _IONBF, 0);
    FILE *config = fopen("/etc/network", "r");
    assert(config && fscanf(config, "%u %x %x", &role, &ip, &server) == 3);
    fclose(config);
    assert(os_udp_open(9000) == -1);
    assert(os_net_configure(0) == -1);
    assert(os_net_configure(ip) == 0);
    int fd = os_udp_open(9000);
    assert(fd >= 0 && os_udp_open(9000) == -1);
    assert(os_udp_open(65536) == -1);
    assert(os_net_configure(ip + 10) == -1);
    assert(os_udp_recv(fd, NULL) == -1);
    assert(os_udp_send(fd, (void*)0xefffffff) == -1);
    assert(os_udp_recv(-1, &reply) == -1);
    p.ip = ip; p.port = 9000; p.length = OS_UDP_MAX + 1;
    assert(os_udp_send(fd, &p) == -1);
    p.length = 0;
    for (unsigned i = 0; i < 33; ++i) send_packet(fd, &p);
    for (unsigned i = 0; i < 32; ++i) assert(os_udp_recv(fd, &reply) == 1 && !reply.length);
    assert(os_udp_recv(fd, &reply) == 0);
    puts("*** net: local bounds, binding and queue checks passed");
    unsigned start = os_milliseconds();
    if (role == 1) {
        unsigned count = 0;
        while (os_milliseconds() - start < 60000) {
            int n = os_udp_recv(fd, &p);
            assert(n >= 0);
            if (n) {
                send_packet(fd, &p);
                if (++count == 300) puts("*** net: responder echoed 300 datagrams");
            }
            os_yield();
        }
        assert(0 && "responder timed out");
    } else {
        p.ip = server;
        for (unsigned seq = 0; seq < 300; ++seq) {
            p.length = seq == 0 ? 0 : seq == 1 ? 1 : seq == 2 ? 1471 : 1472;
            for (unsigned i = 0; i < p.length; ++i) p.data[i] = (seq * 37 + i * 13) & 255;
            unsigned attempt = os_milliseconds();
            send_packet(fd, &p);
            for (;;) {
                if (os_udp_recv(fd, &reply) == 1 && reply.ip == server && reply.port == 9000 &&
                    reply.length == p.length && !memcmp(p.data, reply.data, p.length)) break;
                if (os_milliseconds() - attempt > 200) { send_packet(fd, &p); attempt = os_milliseconds(); }
                assert(os_milliseconds() - start < 30000);
                os_yield();
            }
        }
        os_net_stats stats;
        assert(os_net_get_stats(&stats) == 0 && stats.tx >= 333 && stats.rx >= 332 && stats.arp_requests && stats.dropped >= 6);
        printf("*** net: PASS 300 UDP echoes, lengths 0/1/1471/1472, DMA wrap, ARP, bounds, malformed packets (tx=%u rx=%u drops=%u)\n", stats.tx, stats.rx, stats.dropped);
        assert(os_udp_close(fd) == 0 && os_udp_close(fd) == -1);
    }
    return 0;
}
