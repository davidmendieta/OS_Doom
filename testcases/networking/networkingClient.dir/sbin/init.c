#include "libc.h"
#include "sdl_net_shim.h"
#include "sys.h"
int main(int argc, char** argv) {
    printf("Entering main\n");
    UDPsocket socket = SDLNet_UDP_Open(5000);

    printf("Here opening\n");
    int fd = open("/images/Abel", 0);

    char* buffer = malloc(len(fd) + 1);
    read(fd, buffer, len(fd) + 1);
    write(1, buffer, len(fd) + 1);
    printf("Here packet\n");
    UDPpacket* packet = SDLNet_AllocPacket(len(fd) + 1);
    packet->address.port = htons((uint16_t) 5000); 
    packet->address.host = htonl((uint32_t) 19216811); 
    
    packet->len = len(fd);
    packet->status = 1; 
    printf("Here memcopying\n");
    memcpy(packet->data, buffer, len(fd));
    printf("Here sending\n");
    SDLNet_UDP_Send(socket, 0, packet);
    
    

    shutdown(); 
}
