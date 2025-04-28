#include "libc.h"
#include "sdl_net_shim.h"
#include "sys.h"
int main(int argc, char** argv) {
    //Open a socket and bind
    UDPsocket socket = SDLNet_UDP_Open(5001);

    //Allocate a packet for receiving purposes
    UDPpacket* packet = SDLNet_AllocPacket(4096*3);
    SDLNet_UDP_Recv(socket,packet);
    write(1, packet->data, packet->len);
    printf("\n***\n");

    //Now that I have recieved a packet from the client, send a packet to the client
    int fd2= open("/images/goppert", 0);
    char* buffer = malloc(len(fd2) + 1);
    read(fd2, buffer, len(fd2) + 1);
    UDPpacket* packet2 = SDLNet_AllocPacket(len(fd2) + 1);
    packet2->address.port = packet->address.port;
    packet2->address.host = packet->address.host;
    packet2->len = len(fd2);
    packet2->status = 1; 
    memcpy(packet2->data, buffer, len(fd2));
    SDLNet_UDP_Send(socket, 0, packet2);

    //Recieve another packet from the client
    UDPpacket* packetR = SDLNet_AllocPacket(4096*3);
    SDLNet_UDP_Recv(socket,packetR);
    write(1, packetR->data, packetR->len);
    printf("\n");
    shutdown(); 
}
