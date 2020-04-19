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

void ping::encode_icmp(icmp *icp, int type, int seqno, int id) {
    icp->icmp_type = type;
    icp->icmp_code = 0;
    icp->icmp_cksum = 0;
    icp->icmp_seq = seqno;
    icp->icmp_id = id;
}

void ping::encode_icmp(icmp6_hdr *icp, int type, int seqno, int id) {
    icp->icmp6_type = type;
    icp->icmp6_code = 0;
    icp->icmp6_cksum = 0;
    icp->icmp6_seq = seqno;
    icp->icmp6_id = id;
}

void *ping::sin_get_addr(sockaddr_in *sin) {
    return &(sin->sin_addr);
}

void *ping::sin_get_addr(sockaddr_in6 *sin6) {
    return &(sin6->sin6_addr);
}

void ping::sin_set_family(sockaddr_in *sin, int af) {
    sin->sin_family = af;
}

void ping::sin_set_family(sockaddr_in6 *sin6, int af) {
    sin6->sin6_family = af;
}