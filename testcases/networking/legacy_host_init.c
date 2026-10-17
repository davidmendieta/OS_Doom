#include "libc.h"
#include "sdl_net_shim.h"
#include "sys.h"
int main(int argc, char** argv) {
    printf("Entering main\n");
    UDPsocket socket = SDLNet_UDP_Open(5001);

    printf("Here opening\n");
    UDPpacket* packet = SDLNet_AllocPacket(4096*3);
    SDLNet_UDP_Recv(socket,packet);
    printf("Recieved Packet!\n");
    write(1, packet->data, packet->len + 1);



    printf("Here opening\n");
    int fd2= open("/images/goppert", 0);
    char* buffer = malloc(len(fd2) + 1);
    read(fd2, buffer, len(fd2) + 1);
    //write(1, buffer, len(fd) + 1);
    UDPpacket* packet2 = SDLNet_AllocPacket(len(fd2) + 1);
    packet2->address.port = packet->address.port;
    packet2->address.host = packet->address.host;
    packet2->len = len(fd2);
    packet2->status = 1; 
    memcpy(packet2->data, buffer, len(fd2));
    SDLNet_UDP_Send(socket, 0, packet2);



    UDPpacket* packetR = SDLNet_AllocPacket(4096*3);
    SDLNet_UDP_Recv(socket,packetR);
    printf("Recieved Packet!\n");
    write(1, packetR->data, packetR->len + 1);

    shutdown(); 
}
