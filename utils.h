#ifndef PING_UTILS_H
#define PING_UTILS_H

#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/ip_icmp.h>
#include <netinet/icmp6.h>
#include <zconf.h>

namespace ping {
    struct tag_ipv4;

    struct tag_ipv6;

    unsigned short icmp_cksum(unsigned char *addr, int len);

    uint16_t in_cksum(icmp *addr, int len);

    void encode_icmp(icmp *buffer, int type, int seqno, int id);

    void encode_icmp(icmp6_hdr *buffer, int type, int seqno, int id);

    bool is_ipv4_address(const char *);

    bool is_ipv6_address(const char *);

    int send(int socket, const void * buf, size_t n, int flags, sockaddr *sin, socklen_t len);
}

#endif //PING_UTILS_H
