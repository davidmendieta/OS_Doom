#ifndef OS_NET_H
#define OS_NET_H
#define OS_SYS_NET_CONFIGURE 220
#define OS_SYS_UDP_OPEN 221
#define OS_SYS_UDP_SEND 222
#define OS_SYS_UDP_RECV 223
#define OS_SYS_UDP_CLOSE 224
#define OS_SYS_NET_STATS 225
#define OS_UDP_MAX 1472
#ifndef __ASSEMBLER__
#include <stdint.h>
/* Native ABI: IPs and ports are host-order numeric values (10.0.0.1=0x0a000001).
 * A single foreground application can bind up to eight UDP ports. No routing,
 * fragmentation or DNS. All operations are bounded and nonblocking.
 */
typedef struct { uint32_t ip; uint16_t port, length; uint8_t data[OS_UDP_MAX]; } os_datagram;
typedef struct { uint32_t tx, rx, dropped, arp_requests, arp_replies; } os_net_stats;
#ifdef __cplusplus
extern "C" {
#endif
int os_net_configure(uint32_t address);
int os_udp_open(unsigned port); /* port 0 selects an ephemeral port */
int os_udp_send(int socket, const os_datagram *packet); /* length, -2 retry ARP/TX, -1 invalid */
int os_udp_recv(int socket, os_datagram *packet); /* 1 packet (including empty), 0 none, -1 invalid */
int os_udp_close(int socket);
int os_net_get_stats(os_net_stats *stats);
#ifdef __cplusplus
}
#endif
#endif
#endif
