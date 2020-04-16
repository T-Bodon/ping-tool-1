#ifndef PING_UTILS_H
#define PING_UTILS_H

#include <stdio.h>
#include <errno.h>
#include <sys/time.h>

#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/file.h>

#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>

#include <arpa/inet.h>
#include <strings.h>
#include <zconf.h>

namespace ping {
    unsigned short icmp_cksum(unsigned char *addr, int len);

    uint16_t in_cksum(icmp *addr, int len);

    bool is_ipv4_address(const char *addr);

    bool is_ipv6_address(const char *addr);
}

#endif //PING_UTILS_H
