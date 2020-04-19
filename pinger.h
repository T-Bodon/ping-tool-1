#ifndef PING_PINGER_H
#define PING_PINGER_H

#include <iostream>
#include <poll.h>
#include <string>

#include "utils.h"
#include "ping_error.h"

template<typename IPV = ping::tag_ipv4>
struct pinger {
private:
    using sin_t = typename IPV::sin_t;
    using addr_t = typename IPV::addr_t;
    using icmp_t = typename IPV::icmp_t;

    static const size_t MAXPACKET = 4096;
    char packet[MAXPACKET]{};

    sockaddr_storage sin{};
    sockaddr_storage from{};

    pollfd poll_send{-1, POLLOUT, 0};
    pollfd poll_recv{-1, POLLIN, 0};

    const char *hostname;
    void *sinv_addr;

    const int datalen = 64 - 8;
    int sock = -1;
    int ident;

    int errors = 0;
    int ntransmitted = 0;
    int lost = 0;

public:
    explicit pinger(const char *host, int ttl = -1, bool debug = false, bool dontroute = false) : hostname(host) {
        sin_t *sinv = (sin_t *) (&sin);
        sinv_addr = ping::sin_get_addr(sinv);

        addrinfo hints{AI_ADDRCONFIG, IPV::af, SOCK_RAW, IPV::protocol};
        addrinfo *res = nullptr;
        addrinfo *node = nullptr;
        int code = getaddrinfo(host, nullptr, &hints, &res);
        if (code < 0) {
            throw ping_error("Failed dns resolving");
        }
        for (node = res; node && sock < 0; node = node->ai_next) {
            if (node->ai_family == IPV::af) {
                memcpy(sinv_addr, ping::sin_get_addr((sin_t *) (node->ai_addr)), sizeof(addr_t));
                sock = socket(IPV::af, SOCK_RAW, IPV::protocol);
            }
        }
        freeaddrinfo(res);
        ping::sin_set_family(sinv, IPV::af);
        if (sock < 0) {
            throw ping_error("Failed open connection");
        }
        if (ttl != -1) {
            if (setsockopt(sock, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) < 0) {
                throw ping_error("Failed to set ttl.");
            }
        }
        if (debug) {
            int option = 1;
            if (setsockopt(sock, SOL_SOCKET, SO_DEBUG, &option, sizeof(option)) < 0) {
                throw ping_error("Failed to SO_DEBUG.");
            }
        }
        if (dontroute) {
            int option = 1;
            if (setsockopt(sock, SOL_SOCKET, SO_DONTROUTE, &option, sizeof(option)) < 0) {
                throw ping_error("Failed to SO_DONTROUTE.");
            }
        }
        poll_send.fd = poll_recv.fd = sock;
        ident = getpid() & 0xFFFF;
    }

    explicit pinger(std::string const &host, int ttl = -1, bool debug = false, bool dontroute = false)
            : pinger(host.c_str(), ttl, debug, dontroute) {}

    void ping(size_t cnt) {
        std::cout << "PING: " << hostname << '\n';
        while (cnt-- > 0) {
            int code = ping_impl();
            if (code < 0) errors++;
        }
    }

    void print_stats() const {
        std::cout << "--- " << hostname << " ping stats ---" << std::endl;
        std::cout << ntransmitted << " packets transmitted, " << lost << " packets lost. "
                  << errors << " total errors." << std::endl;
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
        return process_packet(code);
    }

    int send() {
        static u_char outpack[MAXPACKET];
        icmp_t *icp = (icmp_t *) outpack;

        u_char *datap = &outpack[8 + sizeof(struct timeval)];
        for (int i = 8; i < datalen; i++)    /* skip 8 for time */
            *datap++ = i;

        int cc = datalen + 8;            /* skips ICMP portion */
        ping::encode_icmp(icp, IPV::icmp_query_type, ++ntransmitted, ident);
        icp->icmp_cksum = ping::icmp_cksum(outpack, cc);

        if (poll(&poll_send, 1, 1000) != 1) {
            std::cerr << "poll failed";
            return -1;
        }
        int code = sendto(sock, outpack, cc, 0, (sockaddr *) &sin, sizeof(sin_t));

        if (code >= 0 && code != cc) {
            std::cerr << code << '/' << cc << " chars been sent" << std::endl;
            return -1;
        } else if (code < 0) {
            perror("sendto");  // stderr
        }
        return code;
    }

    int receive() {
        socklen_t fromlen = sizeof(sin_t);
        if (poll(&poll_recv, 1, 1000) != 1) {
            std::cerr << "Receive timeout" << std::endl;
            return -1;
        }
        return recvfrom(sock, packet, MAXPACKET, 0, (sockaddr *) (&from), &fromlen);
    }

    int process_packet(int len) {
        ip *pack_ip = (ip *) packet;
        int hlen = pack_ip->ip_hl << 2;

        static void *from_addr = ping::sin_get_addr((sin_t *) &from);
        static char pack_from[IPV::pack_len];
        inet_ntop(IPV::af, from_addr, pack_from, IPV::pack_len);

        if (len < hlen + ICMP_MINLEN) {
            std::cerr << "Too short packet (" << len << " bytes) from " << pack_from << std::endl;
            return -1;
        }

        icmp_t *icp = (icmp_t *) (packet + hlen);
        if (icp->icmp_id != ident) return 0;
        int ttl = -1;
        socklen_t tmp = sizeof(ttl);
        getsockopt(sock, IPPROTO_IP, IP_TTL, &ttl, &tmp);
        std::cout << len - hlen << " bytes"
                  << " from " << pack_from << ": "
                  << "icmp_seq=" << icp->icmp_seq
                  << " ttl=" << ttl
                  << std::endl;
        return 0;
    }
};

#endif //PING_PINGER_H