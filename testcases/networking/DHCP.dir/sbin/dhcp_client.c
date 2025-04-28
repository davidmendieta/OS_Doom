#include "libc.h"      
#include "dhcp.h"
#include "sys.h"
#include "stdint.h"

typedef uint16_t sa_family_t;

#define AF_INET 2
#define SOCK_DGRAM 2

struct sock_addr_in {
    sa_family_t sin_family;   
    uint16_t sin_port;     
    uint32_t sin_addr;   
    char sin_zero[8];  
};

// this is how you get an ip
// it just goes from 192.168.1.1 to .... 254
// we are never going to get to 254 prob, just put there to put there
static uint8_t next = 1;
uint32_t pickip(void) {
    uint32_t prefix = (192u << 24) | (168u << 16) | (1u << 8);
    uint32_t h = next;
    next = (next == 254 ? 1 : next + 1);
    return prefix | h;
}

int main() {
    // open the socket
    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    // bind to port 68
    struct sock_addr_in cli;
    memset(&cli, 0, sizeof(cli));
    cli.sin_family = AF_INET;
    cli.sin_port = htons_(68);
    cli.sin_addr = 0;

    bind(sock, &cli, sizeof(cli));
    // prep the broadcast addr of the server
    struct sock_addr_in srv;
    memset(&srv, 0, sizeof(srv));
    srv.sin_family = AF_INET;
    srv.sin_port = htons_(67);
    srv.sin_addr = 0xFFFFFFFFu;

    // get out mac
    uint8_t client_mac[6];
    get_mac_address(client_mac);

    DHCPHeader dh;
    uint8_t mac[6];
    get_local_mac(mac);

    while (1) {
        uint32_t req_ip = pickip();
        uint32_t xid = req_ip ^ 0xABCDEF;
    
        // send discover to get ip
        build_discover(&dh, req_ip, mac, xid);
        sendto(sock, &dh, sizeof(dh), 0, (void* ) &srv, sizeof(srv));

        // wait for an offer
        DHCPHeader response;
        if (recvfrom(sock, &response, sizeof(response), 0, 0, 0) > 0) {
            uint32_t your_ip = 0;

            // check to make sure xid matches
            if (parse_offer(&response, &your_ip) == 0) {
                change_ip_address(your_ip);
                break;
            }
        }
    }
    return 0;
}