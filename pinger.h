#ifndef PING_PINGER_H
#define PING_PINGER_H

#include <iostream>
#include <string>

#include "utils.h"
#include "ping_error.h"

// TODO:
// stats: timing, loss etc

template<typename IPV = ping::tag_ipv4>
struct pinger {
private:
    using sin_t = typename IPV::sin_t;
    using icmp_t = typename IPV::icmp_t;

    static const size_t MAXPACKET = 4096;
    char packet[MAXPACKET];

    sockaddr_storage sin{};
    sockaddr_storage from{};
    void *sinv_addr;
    const char *hostname;

    int af = AF_INET;
    int sock;
    const int datalen = 64 - 8;
    int ntransmitted = 0;
    int ident;

public:
    explicit pinger(const char *host) {
        sin_t *sinv = (sin_t *) (&sin);
        sinv_addr = ping::sin_get_addr(sinv);

        if (inet_pton(IPV::af, host, sinv_addr) != 0) {  // support preffered af
            hostname = host;
            af = IPV::af;
        } else if (IPV::af != AF_INET && inet_pton(AF_INET, host, sinv_addr) != 0) {  // always support ipv4
            hostname = host;
            af = AF_INET;
        } else {
            hostent *hp = gethostbyname(host);  // resolve host
            if (hp) {
                af = hp->h_addrtype;
                memcpy((caddr_t) sinv_addr, hp->h_addr, hp->h_length);
                hostname = hp->h_name;
            } else {
                throw ping_error(std::string("Unknown host: ") + host);
            }
        }
        ping::sin_set_family(sinv, af);
        sock = socket(af, SOCK_RAW, IPV::protocol);
        if (sock < 0) {
            throw ping_error("Failed open socket");
        }
        ident = getpid() & 0xFFFF;
    }

    explicit pinger(std::string const &host) : pinger(host.c_str()) {}

    void ping(size_t cnt) {
        std::cout << "PING: " << hostname << '\n';
        while (cnt-- > 0) {
            int code = ping_impl();
        }
    }

private:
    [[nodiscard]] int ping_impl() {
        int code = send();
        if (code < 0) {
            std::cerr << "Failed sending" << std::endl;
            return code;
        }
        code = receive();
        if (code < 0) {
            std::cerr << "Failed receiving" << std::endl;
            return code;
        }
        process_packet(code);
        return 0;
    }

    int send() {
        static u_char outpack[MAXPACKET];
        icmp_t *icp = (icmp_t *) outpack;

        u_char *datap = &outpack[8 + sizeof(struct timeval)];
        for (int i = 8; i < datalen; i++)    /* skip 8 for time */
            *datap++ = i;

        int cc = datalen + 8;            /* skips ICMP portion */
        ping::encode_icmp(icp, IPV::icmp_query_type, ntransmitted++, ident, outpack, cc);
        icp->icmp_cksum = ping::icmp_cksum(outpack, cc);

        int code = sendto(sock, outpack, cc, 0, (sockaddr *) &sin, sizeof(sin_t));

        if (code >= 0 && code != cc) {
            std::cerr << code << '/' << cc << " chars been sent" << std::endl;
            return -1;
        } else if (code < 0) {
            perror("sendto");
        }
        return code;
    }

    int receive() {
        socklen_t fromlen = sizeof(sin_t);
        int code = recvfrom(sock, packet, MAXPACKET, 0, (sockaddr *) (&from), &fromlen);
        return code;
    }

    void process_packet(int len) {
        ip *pack_ip = (ip *) packet;
        int hlen = pack_ip->ip_hl << 2;

        static char pack_from[IPV::pack_len];
        inet_ntop(af, sinv_addr, pack_from, IPV::pack_len);

        if (len < hlen + ICMP_MINLEN) {
            std::cerr << "Too short packet (" << len << " bytes) from " << pack_from << std::endl;
        } else {
            icmp_t *icp = (icmp_t *) (packet + hlen);
            if (icp->icmp_id != ident) return;
            std::cout << len - hlen << " bytes from " << pack_from << ": icmp_seq=" << icp->icmp_seq << std::endl;
        }
    }
};

#endif //PING_PINGER_H