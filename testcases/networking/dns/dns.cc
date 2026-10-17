#include "networking.h"
#include "socket/socket.h"

struct DNSRecord {
    const char* name;
    uint32_t    ip;
};

static DNSRecord dns_records[] = {
    { "example.com",  (192<<24)|(168<<16)|(1<<8)|11 },
    { "test.local",   (10<<24)|(0<<16)|(0<<8)|2    },
};

static const int dns_records_count = sizeof(dns_records)/sizeof(dns_records[0]);

static uint16_t read_u16(const uint8_t* p) {
    return (uint16_t(p[0])<<8) | uint16_t(p[1]);
}

static void write_u16(uint8_t* p, uint16_t v) {
    p[0] = v>>8;  p[1] = v & 0xFF;
}

static uint32_t read_u32(const uint8_t* p) {
    return (uint32_t(p[0])<<24)|(uint32_t(p[1])<<16)|(uint32_t(p[2])<<8)|(uint32_t(p[3]));
}

static void write_u32(uint8_t* p, uint32_t v) {
    p[0]=v>>24;  p[1]=(v>>16)&0xFF;  p[2]=(v>>8)&0xFF;  p[3]=v&0xFF;
}

static uint32_t build_dns_response(const uint8_t* req, uint32_t req_len,
                                   uint8_t* resp) {
    if (req_len < 12) return 0; // too small

    // Copy ID + flags + QDCOUNT
    memcpy(resp, req, 4);

    // Set QR=1, Opcode=0, AA=1, RD preserved, others zero, RCODE=0
    resp[2] = 0x81;
    if (req[2] & 1) resp[2] |= 1;
    resp[3] = 0x80;
    write_u16(resp+4, 1);
    uint32_t qdcount = read_u16(req+4);
    if (qdcount != 1) {
        write_u16(resp+6, 0);
        return 12;
    }

    uint32_t pos = 12;
    while (pos < req_len && req[pos] != 0) {
        pos += req[pos] + 1;
    }
    if (pos+5 > req_len) return 0;
    uint32_t qname_end = pos + 1;
    uint16_t qtype  = read_u16(req + qname_end);
    uint16_t qclass = read_u16(req + qname_end + 2);

    memcpy(resp + 12, req + 12, qname_end - 12 + 4);
    uint32_t write_pos = 12 + (qname_end - 12) + 4;

    if (qtype != 1 || qclass != 1) {
        write_u16(resp+6, 0);
        return write_pos;
    }

    // extract the name into a C-string, lowercase
    char namebuf[256];
    uint32_t nb = 0;
    uint32_t rp = 12;
    while (rp < qname_end-1) {
        uint8_t len = req[rp++];
        if (nb > 0) namebuf[nb++] = '.';
        for (uint8_t i = 0; i < len; i++) {
            char c = req[rp++];
            if (c >= 'A' && c <= 'Z') c |= 0x20;
            namebuf[nb++] = c;
        }
    }
    namebuf[nb] = 0;

    // look up
    uint32_t answer_ip = 0;
    for (int i = 0; i < dns_records_count; i++) {
        if (strcmp(namebuf, dns_records[i].name) == 0) {
            answer_ip = dns_records[i].ip;
            break;
        }
    }

    if (answer_ip == 0) {
        // name error
        resp[3] |= 3;
        write_u16(resp+6, 0);
        return write_pos;
    }

    // build one answer
    write_u16(resp+6, 1);
    memcpy(resp + write_pos, req + 12, qname_end - 12);
    write_pos += (qname_end - 12);

    // TYPE A, CLASS IN
    write_u16(resp + write_pos, 1);   write_pos += 2;
    write_u16(resp + write_pos, 1);   write_pos += 2;
    write_u32(resp + write_pos, 60);  write_pos += 4; // TTL = 60s
    write_u16(resp + write_pos, 4);   write_pos += 2;
    write_u32(resp + write_pos, answer_ip);
    write_pos += 4;

    return write_pos;
}

StrongPtr<NetworkInterface> network_interface;

void networkingMain(void) {
    network_interface = StrongPtr<NetworkInterface>::make(19216810);
    network_interface->send_arp_request(19216811);

    auto dns_socket = StrongPtr<sockets::UDPSocket>::make();
    sockets::sockaddr_in addr;
    addr.sa_family      = sockets::AF_INET;
    addr.sin_addr.s_addr = htonl(sockets::INADDR_ANY);
    addr.sin_port       = htons(53);
    dns_socket->bind(&addr);

    // buffer for incoming query + response
    static uint8_t buf[512];
    static uint8_t resp[512];
    sockets::sockaddr_in client;

    while (1) {
        int len = dns_socket->recvfrom((void*)buf, sizeof(buf), &client, 0);
        if (len <= 0) continue;

        uint32_t resp_len = build_dns_response(buf, len, resp);
        if (resp_len > 0) {
            dns_socket->sendto(resp, resp_len, &client);
        }
    }
}
