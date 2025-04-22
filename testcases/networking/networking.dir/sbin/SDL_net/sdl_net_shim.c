#include "../sys/socket.h"

#include <stdint.h>

#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
/* The maximum channels on a a UDP socket */
#define SDLNET_MAX_UDPCHANNELS  32
/* The maximum addresses bound to a single UDP socket channel */
#define SDLNET_MAX_UDPADDRESSES 4

typedef struct {
    uint32_t host;           
    uint16_t port;           
} IPaddress;


typedef struct {
    int channel;        /* The src/dst channel of the packet */
    uint8_t *data;        /* The packet data */
    int len;            /* The length of the packet data */
    int maxlen;         /* The size of the data buffer */
    int status;         /* packet status after sending */
    IPaddress address;  /* The source/dest address of an incoming/outgoing packet */
} UDPpacket;


struct UDP_channel {
    int numbound;
    IPaddress address[SDLNET_MAX_UDPADDRESSES];
};

struct UDPsocket {
    int ready;
    SOCKET socketfd;
    IPaddress address;
    struct UDP_channel binding[SDLNET_MAX_UDPCHANNELS];
    int packetloss;
};