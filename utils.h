#ifndef PING_UTILS_H
#define PING_UTILS_H

#include <arpa/inet.h>
#include <stdio.h>
#include <strings.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/ip_icmp.h>
#include <zconf.h>

namespace ping {
    unsigned short icmp_cksum(unsigned char *addr, int len);

    uint16_t in_cksum(icmp *addr, int len);

    void encode_icmp(icmp *buffer, int type, int seqno, int id);

    bool is_ipv4_address(const char *);

    bool is_ipv6_address(const char *);
}

#endif //PING_UTILS_H
