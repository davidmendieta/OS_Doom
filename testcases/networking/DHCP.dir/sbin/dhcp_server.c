#include "libc.h"      
#include "dhcp.h"
#include "sys.h"
#include "stdint.h"

typedef uint16_t sa_family_t;

#define AF_INET 2
#define SOCK_DGRAM 2

static uint32_t ip_addr[10] = {0};
static uint32_t curr_index = 0;

// mini sockaddr_in
struct sock_addr_in {
    sa_family_t sin_family; // addr family
    uint16_t sin_port; // port
    uint32_t sin_addr; // IP   
    char sin_zero[8]; // padding  
};

int main() {
    // open up udp sock
    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    // bind everything to port 67
    struct sock_addr_in srv;
    memset(&srv, 0, sizeof(srv));
    srv.sin_family = AF_INET;
    srv.sin_port = htons_(67);
    srv.sin_addr = 0;

    bind(sock, &srv, sizeof(srv));

    // grab mac addr
    uint8_t server_mac[6];
    get_mac_address(server_mac);

    // DHCPHeader dh;
    struct sock_addr_in cli;
    int clilen = sizeof(cli);
    // you are now in a loop forever handling requests
    while (1) {
        // get response from client
        if (recvfrom(sock, &dh, sizeof(dh), 0, (void* ) &cli, &clilen) > 0) {
            if (dh.op == DHCP_DISCOVER) {
                // ip they asked for 
                uint32_t req_ip = dh.ciaddr;
                ip_addr[curr_index] = req_ip;
                curr_index = (curr_index + 1) % 10;

                build_offer(&dh, req_ip, mac, dh.xid);
                sendto(sock, &dh, sizeof(dh), 0, (void* ) &cli, sizeof(cli));
            }
        }
    }
    return 0;
}