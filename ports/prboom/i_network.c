/* Native UDP backend and cooperative two-player relay for PrBoom 2.5.0.
 * Protocol/setup defaults adapted from PrBoom's d_server.c (GPL-2.0-or-later).
 * The host pumps its relay in the foreground; no host-side server is needed.
 */
#include <stdio.h>
#include <string.h>
#include <os/net.h>
#include <os/runtime.h>
#include "doomstat.h"
#include "d_net.h"
#include "protocol.h"
#include "i_network.h"
#include "i_system.h"
#include "lprintf.h"

#define PORT 5030
#define HISTORY 128
#define BATCH 8
extern unsigned os_net_role, os_net_ip, os_net_server;
extern void I_PumpSound(void);
int udp_socket = -1;
size_t sentbytes, recvdbytes;
static int server_socket = -1, running, closed, pending, connected;
static unsigned last_receive, joined;
static os_datagram incoming;
static struct client {
    unsigned ip, port, received, sent, heard;
    int ready;
    byte commands[HISTORY][sizeof(ticcmd_t)];
} clients[2];

static int header_valid(const os_datagram *d) {
    if (d->length < sizeof(packet_header_t)) return 0;
    byte check = 0;
    for (unsigned i = 1; i < d->length; ++i) check += d->data[i];
    return check == d->data[0];
}
static int send_to(int fd, unsigned ip, unsigned port, packet_header_t *p, unsigned length) {
    os_datagram d = {0};
    if (length > OS_UDP_MAX) return 0;
    p->checksum = 0;
    for (unsigned i = 1; i < length; ++i) p->checksum += ((byte*)p)[i];
    d.ip = ip; d.port = port; d.length = length;
    memcpy(d.data, p, length);
    int n = os_udp_send(fd, &d);
    if (n >= 0) sentbytes += n;
    return n >= 0; // ARP/TX backpressure: the protocol retries on its next poll.
}
static void signal_client(int index, enum packet_type_e type, unsigned tic) {
    packet_header_t h;
    packet_set(&h, type, tic);
    send_to(server_socket, clients[index].ip, clients[index].port, &h, sizeof(h));
}
static void setup(int index) {
    struct { packet_header_t h; struct setup_packet_s info; } packet = {0};
    static const byte options[GAME_OPTIONS_SIZE] = {
        1,1,0,1,0,1,0,0,0,1,0x12,0x34,0x56,0x78,1,0,0,0,0,128,0,1,1,1,1,0
    };
    packet_set(&packet.h, PKT_SETUP, 0);
    packet.info.players = 2; packet.info.yourplayer = index;
    packet.info.skill = 2; packet.info.episode = packet.info.level = 1;
    packet.info.complevel = best_compatibility;
    packet.info.ticdup = packet.info.extratic = 1;
    memcpy(packet.info.game_options, options, sizeof(options));
    send_to(server_socket, clients[index].ip, clients[index].port, &packet.h, sizeof(packet));
}
static int extra_valid(const os_datagram *d, int player) {
    int values[3];
    if (d->length < sizeof(packet_header_t) + sizeof(values)) return 0;
    memcpy(values, d->data + sizeof(packet_header_t), sizeof(values));
    if ((unsigned)values[1] >= 2 || (player >= 0 && values[1] != player) ||
        values[2] < 0 || (unsigned)values[2] != d->length - sizeof(packet_header_t) - sizeof(values)) return 0;
    if (values[0] == nm_plcolour) {
        int colour;
        if (values[2] != sizeof(colour)) return 0;
        memcpy(&colour, d->data + sizeof(packet_header_t) + sizeof(values), sizeof(colour));
        return colour >= 0 && colour <= 255; // palette index, not a player number
    }
    return values[0] == nm_savegamename && values[2] < 24;
}
static void stop_server(void) {
    if (!closed) puts("*** net: peer disconnected; ending network session");
    closed = 1;
    for (int i = 0; i < 2; ++i) if (clients[i].port) signal_client(i, PKT_DOWN, 0);
}
void I_ServerPoll(void) {
    if (server_socket < 0) return;
    os_datagram d;
    for (unsigned budget = 0; budget < 32 && os_udp_recv(server_socket, &d) == 1; ++budget) {
        if (!header_valid(&d)) continue;
        packet_header_t *h = (void*)d.data;
        byte *body = d.data + sizeof(*h);
        unsigned size = d.length - sizeof(*h), tic = doom_ntohl(h->tic);
        int index = -1;
        for (int i = 0; i < 2; ++i) if (clients[i].ip == d.ip && clients[i].port == d.port) index = i;
        if (h->type == PKT_INIT && size == 2) {
            if (index < 0 && !running && !closed) {
                index = d.ip == os_net_ip ? 0 : 1;
                if (clients[index].port) continue;
                clients[index].ip = d.ip; clients[index].port = d.port;
                printf("*** net: player %d connected\n", index + 1);
            }
            if (index >= 0 && !closed) setup(index);
        }
        if (index < 0) continue;
        struct client *c = &clients[index];
        c->heard = os_milliseconds();
        if (closed) { signal_client(index, PKT_DOWN, 0); continue; }
        switch (h->type) {
        case PKT_GO:
            if (size != 1 || body[0] != index) break;
            c->ready = 1;
            if (clients[0].ready && clients[1].ready) {
                if (!running) puts("*** net: two-player cooperative game started");
                running = 1;
                signal_client(0, PKT_GO, 0); signal_client(1, PKT_GO, 0);
            }
            break;
        case PKT_TICC:
            if (!running || size < 2 || body[1] != index || !body[0] || body[0] > BACKUPTICS ||
                size != 2 + body[0] * sizeof(ticcmd_t) || tic > 0x7fffffff - body[0]) break;
            if (tic > c->received) { signal_client(index, PKT_RETRANS, c->received); break; }
            if (tic + body[0] > clients[1-index].received + HISTORY / 2) break;
            for (unsigned t = c->received; t < tic + body[0]; ++t)
                memcpy(c->commands[t % HISTORY], body + 2 + (t - tic) * sizeof(ticcmd_t), sizeof(ticcmd_t));
            if (tic + body[0] > c->received) c->received = tic + body[0];
            break;
        case PKT_RETRANS:
            if (running && size == 1 && body[0] == index && tic <= c->sent && c->sent - tic < HISTORY / 2)
                c->sent = tic;
            break;
        case PKT_EXTRA:
            if (extra_valid(&d, index) && tic <= c->received)
                for (int i = 0; i < 2; ++i) if (clients[i].port)
                    send_to(server_socket, clients[i].ip, clients[i].port, h, d.length);
            break;
        case PKT_QUIT:
            if (size == 1 && body[0] == index) stop_server();
            break;
        default: break;
        }
    }
    if (!running || closed) return;
    unsigned low = clients[0].received < clients[1].received ? clients[0].received : clients[1].received;
    for (int i = 0; i < 2; ++i) {
        struct client *c = &clients[i];
        if (os_milliseconds() - c->heard > 15000) { stop_server(); return; }
        if (c->sent >= low) continue;
        byte buffer[sizeof(packet_header_t) + 1 + BATCH * (1 + 2 * (1 + sizeof(ticcmd_t)))];
        packet_header_t *h = (void*)buffer;
        unsigned first = c->sent ? c->sent - 1 : 0;
        unsigned count = low - first;
        if (count > BATCH) count = BATCH;
        packet_set(h, PKT_TICS, first);
        byte *p = buffer + sizeof(*h);
        *p++ = count;
        for (unsigned t = first; t < first + count; ++t) {
            *p++ = 2;
            for (int j = 0; j < 2; ++j) {
                *p++ = j;
                memcpy(p, clients[j].commands[t % HISTORY], sizeof(ticcmd_t));
                p += sizeof(ticcmd_t);
            }
        }
        if (send_to(server_socket, c->ip, c->port, h, p - buffer)) c->sent = first + count;
    }
}
void I_InitNetwork(void) {
    if (os_net_configure(os_net_ip) < 0) I_Error("Cannot configure native IPv4");
    joined = last_receive = os_milliseconds();
    if (os_net_role == 1) {
        server_socket = os_udp_open(PORT);
        if (server_socket < 0) I_Error("Cannot bind Doom server UDP port");
        puts("*** net: hosting on 10.0.0.1:5030; waiting for player 2");
    }
}
int I_Socket(unsigned short port) {
    int fd = os_udp_open(port);
    if (fd < 0) I_Error("Cannot open Doom UDP client");
    return fd;
}
int I_ConnectToServer(const char *unused) { (void)unused; return 0; }
void I_SendPacket(packet_header_t *p, size_t size) {
    send_to(udp_socket, os_net_server, PORT, p, size);
    I_ServerPoll();
}
/* d_client trusts the packet layout. Validate it here before handing it over. */
static int client_valid(const os_datagram *d) {
    if (d->ip != os_net_server || d->port != PORT || !header_valid(d)) return 0;
    const packet_header_t *h = (const void*)d->data;
    const byte *b = d->data + sizeof(*h);
    unsigned size = d->length - sizeof(*h), tic = doom_ntohl(h->tic);
    switch (h->type) {
    case PKT_SETUP: {
        if (size != sizeof(struct setup_packet_s)) return 0;
        const struct setup_packet_s *s = (const void*)b;
        return s->players == 2 && s->yourplayer == os_net_role - 1 && s->skill <= 4 &&
            s->episode == 1 && s->level == 1 && !s->deathmatch && s->complevel == best_compatibility &&
            s->ticdup == 1 && s->extratic <= 1 && !s->numwads;
    }
    case PKT_GO: case PKT_DOWN: return !size;
    case PKT_RETRANS: return !size && tic <= (unsigned)maketic && (unsigned)maketic - tic <= BACKUPTICS;
    case PKT_EXTRA: return extra_valid(d, -1) && tic <= (unsigned)maketic;
    case PKT_TICS:
        if (!size || !b[0] || b[0] > BACKUPTICS || size != 1 + b[0] * (1 + 2 * (1 + sizeof(ticcmd_t))) ||
            tic > (unsigned)maketic || tic + b[0] > (unsigned)maketic) return 0;
        for (unsigned t = 0, offset = 1; t < b[0]; ++t, offset += 19)
            if (b[offset] != 2 || b[offset + 1] != 0 || b[offset + 10] != 1) return 0;
        return 1;
    default: return 0;
    }
}
static int receive(void) {
    I_ServerPoll();
    if (pending) return 1;
    for (unsigned budget = 0; budget < 32 && os_udp_recv(udp_socket, &incoming) == 1; ++budget) {
        if (!client_valid(&incoming)) continue;
        last_receive = os_milliseconds();
        if (((packet_header_t*)incoming.data)->type == PKT_GO) connected = 1;
        recvdbytes += incoming.length;
        return pending = 1;
    }
    if (!connected && os_milliseconds() - joined > 120000) I_Error("Timed out waiting for second Doom player");
    if (connected && os_milliseconds() - last_receive > 15000) {
        last_receive = os_milliseconds();
        incoming.length = sizeof(packet_header_t);
        packet_set((void*)incoming.data, PKT_DOWN, 0);
        puts("*** net: server timed out; ending network session");
        return pending = 1;
    }
    return 0;
}
size_t I_GetPacket(packet_header_t *buffer, size_t capacity) {
    if (!receive()) return 0;
    pending = 0;
    if (incoming.length > capacity) return 0;
    memcpy(buffer, incoming.data, incoming.length);
    return incoming.length;
}
void I_WaitForPacket(int milliseconds) {
    unsigned start = os_milliseconds();
    do {
        if (receive()) return;
        I_PumpSound(); os_yield();
    } while (os_milliseconds() - start < (unsigned)milliseconds);
}
