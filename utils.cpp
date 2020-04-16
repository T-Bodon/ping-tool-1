#include "utils.h"

unsigned short ping::icmp_cksum(unsigned char *addr, int len) {
    int sum = 0;
    unsigned short answer = 0;
    unsigned short *wp;

    for (wp = (unsigned short *) addr; len > 1; wp++, len -= 2)
        sum += *wp;

    /* Take in an odd byte if present */
    if (len == 1) {
        *(unsigned char *) &answer = *(unsigned char *) wp;
        sum += answer;
    }

    sum = (sum >> 16) + (sum & 0xffff);    /* add high 16 to low 16 */
    sum += (sum >> 16);        /* add carry */
    answer = ~sum;        /* truncate to 16 bits */
    return answer;
}

uint16_t ping::in_cksum(icmp *addr, int len) {
    int nleft = len;
    u_short *w = reinterpret_cast<u_short *>(addr);
    u_short answer;
    int sum = 0;

    /*
     *  Our algorithm is simple, using a 32 bit accumulator (sum),
     *  we add sequential 16 bit words to it, and at the end, fold
     *  back all the carry bits from the top 16 bits into the lower
     *  16 bits.
     */
    while (nleft > 1) {
        sum += *w++;
        nleft -= 2;
    }

    /* mop up an odd byte, if necessary */
    if (nleft == 1) {
        u_short u = 0;

        *(u_char *) (&u) = *(u_char *) w;
        sum += u;
    }

    /*
     * add back carry outs from top 16 bits to low 16 bits
     */
    sum = (sum >> 16) + (sum & 0xffff);    /* add hi 16 to low 16 */
    sum += (sum >> 16);            /* add carry */
    answer = ~sum;                /* truncate to 16 bits */
    return (answer);
}

bool ping::is_ipv4_address(const char *addr) {
    sockaddr_in sa{};
    return inet_pton(AF_INET, addr, &(sa.sin_addr)) != 0;
}

bool ping::is_ipv6_address(const char *addr) {
    sockaddr_in sa{};
    return inet_pton(AF_INET6, addr, &(sa.sin_addr)) != 0;
}