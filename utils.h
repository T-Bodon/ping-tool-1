#ifndef PING_UTILS_H
#define PING_UTILS_H

#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/ip_icmp.h>
#include <netinet/icmp6.h>
#include <zconf.h>

#include <arpa/inet.h>
#include <netdb.h>

namespace ping {
    struct tag_ipv4 {
        using sin_t = sockaddr_in;
        using addr_t = in_addr;
        using icmp_t = icmp;
        const static int af = AF_INET;
        const static int protocol = IPPROTO_ICMP;
        const static int icmp_query_type = ICMP_ECHO;
        const static socklen_t pack_len = INET_ADDRSTRLEN;
    };

    struct tag_ipv6 {
        using sin_t = sockaddr_in6;
        using addr_t = in6_addr;
        using icmp_t = icmp;
        const static int af = AF_INET6;
        const static int protocol = IPPROTO_ICMPV6;
        const static int icmp_query_type = ICMP6_ECHO_REQUEST;
        const static socklen_t pack_len = INET6_ADDRSTRLEN;
    };

    unsigned short icmp_cksum(unsigned char *addr, int len);

    void encode_icmp(icmp *buffer, int type, int seqno, int id);

    void encode_icmp(icmp6_hdr *buffer, int type, int seqno, int id);

    void *sin_get_addr(sockaddr_in *);

    void *sin_get_addr(sockaddr_in6 *);

    void sin_set_family(sockaddr_in *, int);

    void sin_set_family(sockaddr_in6 *, int);
}

#endif //PING_UTILS_H
