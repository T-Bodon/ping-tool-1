#ifndef PING_PINGER_H
#define PING_PINGER_H

#include <iostream>
#include <string>

#include "utils.h"
#include "ping_error.h"

// TODO:

// stats: timing, loss etc
// ipv4 ipv6 switcher

template<typename T = ping::tag_ipv4>
struct pinger {
    static const size_t MAXPACKET = 4096;
    char packet[MAXPACKET];

    sockaddr_in sin;
    sockaddr_in from;
    const char *hostname;

    int sock;
    int datalen = 64 - 8;
    int ntransmitted = 0;
    int ident;

    explicit pinger(const char *host) {
        sin.sin_family = AF_INET;
        if (inet_pton(AF_INET, host, &(sin.sin_addr)) != 0) {
            hostname = host;
        } else {
            hostent *hp = gethostbyname(host);
            if (hp) {
                sin.sin_family = hp->h_addrtype;
                memcpy((caddr_t) &sin.sin_addr, hp->h_addr, hp->h_length);
                hostname = hp->h_name;
            } else {
                throw ping_error(std::string("Unknown host: ") + host);
            }
        }
        sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
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
        icmp *icp = (icmp *) outpack;
        ping::encode_icmp(icp, ICMP_ECHO, ntransmitted++, ident);

        // u_char *datap = &outpack[8 + sizeof(struct timeval)];
        //for (int i = 8; i < datalen; i++)    /* skip 8 for time */
        //    *datap++ = i;

        int cc = datalen + 8;            /* skips ICMP portion */
        icp->icmp_cksum = ping::icmp_cksum(outpack, cc);
        int code = sendto(sock, outpack, cc, 0, (sockaddr *) &sin, sizeof(sockaddr_in));
        if (code >= 0 && code != cc) {
            std::cerr << code << '/' << cc << " chars been sent" << std::endl;
            return -1;
        }
        return code;
    }

    int receive() {
        socklen_t fromlen = sizeof(from);
        int code = recvfrom(sock, packet, MAXPACKET, 0, reinterpret_cast<sockaddr *>(&from), &fromlen);
        return code;
    }

    void process_packet(int len) {
        ip *pack_ip = (ip *) packet;
        int hlen = pack_ip->ip_hl << 2;
        const char *pack_from = inet_ntoa(from.sin_addr);
        if (len < hlen + ICMP_MINLEN) {
            std::cerr << "Too short packet (" << len << " bytes) from " << pack_from << std::endl;
        } else {
            icmp *icp = (icmp *) (packet + hlen);
            if (icp->icmp_id != ident) return;
            std::cout << len - hlen << " bytes from " << pack_from << ": icmp_seq=" << icp->icmp_seq << std::endl;
        }
    }
};

template<>
struct pinger<ping::tag_ipv6> {
    static const size_t MAXPACKET = 4096;
    char packet[MAXPACKET];

    sockaddr_in6 sin6;
    sockaddr_in6 from;
    const char *hostname;

    int sock;
    int datalen = 64 - 8;
    int ntransmitted = 0;
    int ident;

    explicit pinger(const char *host) {
        sin6.sin6_family = AF_INET6;
        sin6.sin6_scope_id = 0;
        sin6.sin6_flowinfo = 0;

        if (inet_pton(AF_INET6, host, &(sin6.sin6_addr)) != 0) {
            hostname = host;
        } else {
            hostent *hp = gethostbyname(host);
            if (hp) {
                sin6.sin6_family = hp->h_addrtype;
                memcpy((caddr_t) &sin6.sin6_addr, hp->h_addr, hp->h_length);
                hostname = hp->h_name;
            } else {
                throw ping_error(std::string("Unknown host: ") + host);
            }
        }
        sock = socket(sin6.sin6_family, SOCK_RAW, IPPROTO_ICMPV6);
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
        icmp6_hdr *icp = (icmp6_hdr *) outpack;

        ping::encode_icmp(icp, ICMP6_ECHO_REQUEST, ntransmitted++, ident);

        // u_char *datap = &outpack[8 + sizeof(struct timeval)];
        // for (int i = 8; i < datalen; i++)    /* skip 8 for time */
        //    *datap++ = i;

        int cc = datalen + 8;
        icp->icmp6_cksum = ping::icmp_cksum(outpack, cc);
        //int code = sendto(sock, outpack, cc, 0, (struct sockaddr *)(sin6), sizeof(sockaddr_in6));
        int code = ping::send(sock, outpack, cc, 0, (sockaddr *) &sin6, sizeof(sockaddr_in6));

        if (code >= 0 && code != cc) {
            std::cerr << code << '/' << cc << " chars been sent" << std::endl;
            return -1;
        } else if (code < 0) {
            perror("sendto");
        }
        return code;
    }

    int receive() {
        socklen_t fromlen = sizeof(from);
        int code = recvfrom(sock, packet, MAXPACKET, 0, reinterpret_cast<sockaddr *>(&from), &fromlen);
        return code;
    }

    void process_packet(int len) {
        ip *pack_ip = (ip *) packet;
        int hlen = pack_ip->ip_hl << 2;
        const char *pack_from = hostname;
        if (len < hlen + ICMP_MINLEN) {
            std::cerr << "Too short packet (" << len << " bytes) from " << pack_from << std::endl;
        } else {
            icmp6_hdr *icp = (icmp6_hdr *) (packet + hlen);
            if (icp->icmp6_id != ident) return;
            std::cout << len - hlen << " bytes from " << pack_from << ": icmp_seq=" << icp->icmp6_seq << std::endl;
        }
    }
};


#endif //PING_PINGER_H
