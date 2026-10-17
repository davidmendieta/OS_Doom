#include "libc.h"
#include "sdl_net_shim.h"
#include "sys.h"
int main(int argc, char** argv) {
    printf("Entering main\n");
    UDPsocket socket = SDLNet_UDP_Open(5000);

    printf("Here opening\n");
    int fd = open("/images/Gheith", 0);

    char* buffer = malloc(len(fd) + 1);
    read(fd, buffer, len(fd) + 1);

    
    UDPpacket* packet = SDLNet_AllocPacket(len(fd) + 1);
    packet->address.port = htons((uint16_t) 5001); 
    packet->address.host = htonl((uint32_t) 19216811); 
    
    packet->len = len(fd);
    packet->status = 1; 
   
    memcpy(packet->data, buffer, len(fd));
    
    SDLNet_UDP_Send(socket, 0, packet);
    
   


    UDPpacket* packetR = SDLNet_AllocPacket(4096*3);
    SDLNet_UDP_Recv(socket,packetR);
    printf("Recieved Packet!\n");
    write(1, packetR->data, packetR->len + 1);

    
   

    int fd2 = open("/images/Abel", 0);
    read(fd2, buffer, len(fd2) + 1);
    UDPpacket* packet2 = SDLNet_AllocPacket(len(fd2) + 1);
    packet2->address.port = packetR->address.port;
    packet2->address.host = packetR->address.host;
    packet2->len = len(fd2);
    packet2->status = 1; 
    memcpy(packet2->data, buffer, len(fd));
    SDLNet_UDP_Send(socket, 0, packet2);
    
   



    shutdown(); 
}
