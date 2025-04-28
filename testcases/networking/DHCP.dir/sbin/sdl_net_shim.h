

//Just a poorly built out shim for testing purposes only


#include "heap.h"
#include "libc.h"
#include "socket.h"
#include "stdint.h"

#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
/* The maximum channels on a a UDP socket */
#define SDLNET_MAX_UDPCHANNELS 32
/* The maximum addresses bound to a single UDP socket channel */
#define SDLNET_MAX_UDPADDRESSES 4

typedef struct {
    uint32_t host;
    uint16_t port;
} IPaddress;

typedef struct {
    int channel;       /* The src/dst channel of the packet */
    uint8_t *data;     /* The packet data */
    int len;           /* The length of the packet data */
    int maxlen;        /* The size of the data buffer */
    int status;        /* packet status after sending */
    IPaddress address; /* The source/dest address of an incoming/outgoing packet */
} UDPpacket;

typedef struct {
    int numbound;
    IPaddress address[SDLNET_MAX_UDPADDRESSES];
} UDP_channel;

typedef struct {
    int ready;
    SOCKET socketfd;
    IPaddress address;
    UDP_channel binding[SDLNET_MAX_UDPCHANNELS];
    int packetloss;
} _UDPsocket;

typedef struct _UDPsocket* UDPsocket;

extern UDPpacket *SDLNet_AllocPacket(int size);

extern UDPsocket SDLNet_UDP_Open(uint16_t port);

extern void SDLNet_FreePacket(UDPpacket *packet);
extern int SDLNet_UDP_RecvV(UDPsocket sockptr, UDPpacket **packets);

extern int SDLNet_UDP_Recv(UDPsocket sock, UDPpacket *packet);




extern int SDLNet_UDP_SendV(UDPsocket sockptr, UDPpacket **packets, int npackets);

extern int SDLNet_UDP_Send(UDPsocket sock, int channel, UDPpacket *packet);