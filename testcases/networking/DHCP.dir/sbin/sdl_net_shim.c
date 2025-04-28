#include "sdl_net_shim.h"

UDPpacket *SDLNet_AllocPacket(int size) {
    UDPpacket *packet;
    packet = (UDPpacket *)malloc(sizeof(*packet));
    if (packet) {
        packet->maxlen = size;
        packet->data = (uint8_t *)malloc(size);
        if (packet->data) {
            packet->len = 0;
            packet->status = 0;
            packet->channel = -1;
            packet->address.host = 0;
            packet->address.port = 0;
            return packet;
        } else {
            free(packet);
        }
    }
    return 0;
}

UDPsocket SDLNet_UDP_Open(uint16_t port) {
    printf("Opening\n");
    _UDPsocket *sock = (malloc(sizeof(_UDPsocket)));
    struct sockaddr_in sock_addr;
    //socklen_t sock_len;

    /* Allocate a UDP socket structure */

    if (!sock) {
        return 0;
    }
    printf(" ");

    // memset(sock, 0, sizeof(*sock));
    // memset(&sock_addr, 0, sizeof(sock_addr));
    sock->ready = 1;
    printf("opening socket4");
    printf(" ");
    /* Open the socket */
    
    sock->socketfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock->socketfd == INVALID_SOCKET) {
        free(sock);
        return 0;
    }
    printf("opening socket5");
    printf(" ");
    /* Bind locally, if appropriate */
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_addr.s_addr = INADDR_ANY;
    sock_addr.sin_port = htons(port);
    /* Bind the socket for listening */
    printf("opening socket6");
    printf(" ");
    if (bind(sock->socketfd, (struct sockaddr *)&sock_addr, sizeof(sock_addr)) == SOCKET_ERROR) {
        close(sock->socketfd);
        free(sock);
        return 0;
    }
    printf("opening socket7\n");
    printf(" \n");
    printf("opening socket8\n");

    /* Get the bound address and port */
    //sock_len = sizeof(sock_addr);
    //printf("opening socket10\n");
    // if (getsockname(sock->socketfd, (struct sockaddr *)&sock_addr, &sock_len) < 0) {
    //     printf("opening socket9\n");
    //     close(sock->socketfd);
    //     free(sock);
    //     return 0;
    // }
    // printf("opening socket8");
    // /* Fill in the channel host address */
    // sock->address.host = sock_addr.sin_addr.s_addr;
    // sock->address.port = sock_addr.sin_port;
    return (UDPsocket) sock;
}

void SDLNet_FreePacket(UDPpacket *packet) {
    if (packet) {
        free(packet->data);
        free(packet);
    }
}

int SDLNet_UDP_RecvV(UDPsocket sockptr, UDPpacket **packets) {
    //int numrecv, i, j;
    //struct UDP_channel *binding;
    socklen_t sock_len;
    struct sockaddr_in sock_addr;
    _UDPsocket* sock = (_UDPsocket*) sockptr;

    if (sock == 0) {
        return (0);
    }
    int numrecv = 0;
    while (packets[numrecv]) {
        UDPpacket *packet;

        packet = packets[numrecv];

        sock_len = sizeof(sock_addr);
        packet->status = recvfrom(sock->socketfd, packet->data, packet->maxlen, 0,
                                  (struct sockaddr *)&sock_addr, &sock_len);
        if (packet->status >= 0) {
            packet->len = packet->status;
            packet->address.host = sock_addr.sin_addr.s_addr;
            packet->address.port = sock_addr.sin_port;
            packet->channel = -1;
            numrecv++;
        } else {
            packet->len = 0;
            break;
        }
    }

    sock->ready = 0;

    return numrecv;
}

int SDLNet_UDP_Recv(UDPsocket sock, UDPpacket *packet) {
    UDPpacket *packets[2];
    /* Receive a packet array of 1 */
    packets[0] = packet;
    packets[1] = 0;
    return (SDLNet_UDP_RecvV(sock, packets));
}




int SDLNet_UDP_SendV(UDPsocket sockptr, UDPpacket **packets, int npackets) {
    if (!sockptr)  {
        return 0;
    }
    struct sockaddr_in dst;
    socklen_t dstlen = sizeof(dst);
    int sent = 0;
    _UDPsocket* sock = (_UDPsocket*) sockptr;

    for (int i = 0; i < npackets && packets[i]; ++i) {
        UDPpacket *packet = packets[i];

        dst.sin_family = AF_INET;
        dst.sin_addr.s_addr = packet->address.host;
        dst.sin_port = packet->address.port;

        int sentstatus = sendto(sock->socketfd, packet->data, packet->len, 0, (struct sockaddr *)&dst, dstlen);

        if (sentstatus >= 0) {
            packet->status = sentstatus;
            sent++;
        }
    }
    return sent;
}

int SDLNet_UDP_Send(UDPsocket sock, int channel, UDPpacket *packet) {
    return (SDLNet_UDP_SendV(sock, &packet, 1));
}
