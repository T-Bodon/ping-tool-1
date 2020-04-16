#include <iostream>
#include <string>

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

// TODO:
// high class abstraction
// utils
// constructors

struct ping_caller {
    int sock;
    int datalen = 64 - 8;
    int timing = 0;
    const char *hostname;
    hostent *hp;
    protoent *proto;
    int ntransmitted = 0;
    struct timezone tz;
    sockaddr whereto{};
    sockaddr_in from;
    int ident;
    static const size_t MAXPACKET = 4096;

    char packet[MAXPACKET];

    explicit ping_caller(const char *host) {
        sockaddr_in *sain = (sockaddr_in *) &whereto;
        bzero((char *) &whereto, sizeof(struct sockaddr));
        sain->sin_family = AF_INET;
        sain->sin_addr.s_addr = inet_addr(host);
        if (sain->sin_addr.s_addr != (unsigned) -1) {
            hostname = host;
        } else {
            hp = gethostbyname(host);
            if (hp) {
                sain->sin_family = hp->h_addrtype;
                bcopy(hp->h_addr, (caddr_t) &sain->sin_addr, hp->h_length);
                hostname = hp->h_name;
            } else {
                std::cerr << "Unknown host: " << host << std::endl;
                return;
            }
        }
        std::cout << hostname << '\n';
        if ((proto = getprotobyname("icmp")) == nullptr) {
            std::cerr << "Unknown protocol: icmp" << std::endl;
            return;
        }
        if ((sock = socket(AF_INET, SOCK_RAW, proto->p_proto)) < 0) {
            std::cerr << "Failed open socket" << errno << std::endl;
            exit(10);
        }
        ping();
        std::cout << "OK";
        for (size_t ttt = 0; ttt < 1; ttt++) {
            int len = sizeof(packet);
            socklen_t fromlen = sizeof(from);
            int cc;
            std::cout << "HERE";
            if ((cc = recvfrom(sock, packet, len, 0, reinterpret_cast<sockaddr *>(&from), &fromlen)) < 0) {
                std::cout << "FAIL";
            }
            pr_pack(packet, cc, &from);
        }
    }

    void ping() {
        static u_char outpack[MAXPACKET];
        icmp *icp = (struct icmp *) outpack;
        int i, cc;
        timeval *tp = (struct timeval *) &outpack[8];
        u_char *datap = &outpack[8 + sizeof(struct timeval)];

        icp->icmp_type = ICMP_ECHO;
        icp->icmp_code = 0;
        icp->icmp_cksum = 0;
        icp->icmp_seq = ntransmitted++;
        ident = getpid() & 0xFFFF;
        icp->icmp_id = ident;

        cc = datalen + 8;            /* skips ICMP portion */
        if (timing) gettimeofday(tp, &tz);
        for (i = 8; i < datalen; i++)    /* skip 8 for time */
            *datap++ = i;

        icp->icmp_cksum = in_cksum(icp, cc);
        i = sendto(sock, outpack, cc, 0, &whereto, sizeof(struct sockaddr));

        if (i < 0 || i != cc) {
            if (i < 0) perror("sendto");
            printf("ping: wrote %s %d chars, ret=%dn",
                   hostname, cc, i);
            fflush(stdout);
        }
    }

    static bool is_ipv4_address(const char *addr) {
        sockaddr_in sa{};
        return inet_pton(AF_INET, addr, &(sa.sin_addr)) != 0;
    }

    static bool is_ipv6_address(const char *addr) {
        sockaddr_in sa{};
        return inet_pton(AF_INET6, addr, &(sa.sin_addr)) != 0;
    }

    uint16_t in_cksum(icmp *addr, int len) {
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

    void pr_pack(char *buf, int cc, sockaddr_in *from) {
        ip *ip = (struct ip *) buf;
        int hlen = ip->ip_hl << 2;

        if (cc < hlen + ICMP_MINLEN) {
            std::cerr << "Packet too short (" << cc << " bytes) from " << inet_ntoa(from->sin_addr);
            return;
        }
        cc -= hlen;
        icmp * icp = (icmp *) (buf + hlen);
        if (icp->icmp_id != ident) return;

        std::cout << cc << " bytes from " << inet_ntoa(from->sin_addr) << ": icmp_seq=" << icp->icmp_seq << '\n';
    }
};

int main() {
    ping_caller("google.com");
    // ping_caller("etbenewnwnr.cur");
    // ping_caller("64.233.164.101");
    // ping_caller("2a00:1450:4010:c07::64");
    return 0;
}