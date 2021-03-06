#ifndef PING_PINGER_H
#define PING_PINGER_H

#include <chrono>
#include <iostream>
#include <signal.h>
#include <string>

#include "utils.h"
#include "ping_error.h"

namespace {
    volatile sig_atomic_t quit_signal_flag = 0;

    void signal_handler([[maybe_unused]] int signal) {
        quit_signal_flag = 1;
    }
}

template<typename IPV = ping::tag_ipv4>
struct pinger {
private:
    using sin_t = typename IPV::sin_t;
    using addr_t = typename IPV::addr_t;
    using icmp_t = typename IPV::icmp_t;
    using time_t = std::chrono::steady_clock::time_point;

    static const size_t MAXPACKET = 4096;
    char packet[MAXPACKET];

    sockaddr_storage sin{};
    sockaddr_storage from{};

    pollfd poll_send{-1, POLLOUT, 0};
    pollfd poll_recv{-1, POLLIN, 0};

    const char *hostname;
    void *sinv_addr;

    const int timeout;  // timeout in ms for send and receive; 1000 by default or -1 for no timeout
    const int datalen;
    int sock = -1;
    int ident;

    uint64_t total_time = 0;
    int32_t errors = 0;
    int32_t transmitted = 0;
    int32_t lost = 0;

    const bool quiet = false;
    const bool timing = true;

public:
    explicit pinger(const char *host, int timeout = 1000, size_t datalen = 64 - 8, int ttl = -1, bool quiet = false,
                    bool debug = false, bool dontroute = false)
            : hostname(host), timeout(timeout), datalen(datalen), quiet(quiet),
              timing(datalen >= sizeof(time_t)) {
        if (datalen + 8 > MAXPACKET) {
            throw ping_error("Too large packet size");
        }

        sin_t *sinv = (sin_t *) (&sin);
        sinv_addr = ping::sin_get_addr(sinv);
        ping::sin_set_family(sinv, IPV::af);

        // resolve
        addrinfo hints{AI_ADDRCONFIG, IPV::af, SOCK_RAW, IPV::protocol};
        addrinfo *res = nullptr;
        addrinfo *node = nullptr;
        int code = getaddrinfo(host, nullptr, &hints, &res);
        if (code < 0) {
            throw ping_error("Failed dns resolving");
        }
        for (node = res; node; node = node->ai_next) {
            if (node->ai_family == IPV::af) {
                memcpy(sinv_addr, ping::sin_get_addr((sin_t *) (node->ai_addr)), sizeof(addr_t));
                break;
            }
        }
        freeaddrinfo(res);

        // socket with opts
        sock = socket(IPV::af, SOCK_RAW, IPV::protocol);
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

    explicit pinger(std::string const &host, int timeout = 1000, size_t datalen = 64 - 8, int ttl = -1,
                    bool quiet = false, bool debug = false, bool dontroute = false)
            : pinger(host.c_str(), timeout, datalen, ttl, quiet, debug, dontroute) {}

    void ping(int cnt) {
        if (cnt == -1) {
            ping();
        } else {
            signal(SIGINT, ::signal_handler);
            std::cout << "PING: " << hostname << '\n';
            while (cnt > 0 && !quit_signal_flag) {
                int code = ping_impl();
                if (code != 0) errors++;
                else cnt--;
            }
            print_stats();
        }
    }

    void ping() {
        signal(SIGINT, ::signal_handler);
        std::cout << "PING: " << hostname << '\n';
        while (!quit_signal_flag) {
            int code = ping_impl();
            if (code < 0) errors++;
        }
        print_stats();
    }

    ~pinger() {
        close(sock);
    }

private:
    [[nodiscard]] int ping_impl() {
        int code = send();
        if (code < 0) {
            if (!quiet) std::cerr << "Failed sending" << std::endl;
            return code;
        }
        code = receive();
        if (code < 0) {
            if (!quiet) std::cerr << "Failed receiving" << std::endl;
            return code;
        }
        return process_packet(code);
    }

    [[nodiscard]] int send() {
        static u_char outpack[MAXPACKET];
        icmp_t *icp = (icmp_t *) outpack;

        if (timing) {
            time_t cur_time = std::chrono::steady_clock::now();
            memcpy(&outpack[8], &cur_time, sizeof(time_t));
        }

        u_char *datap = &outpack[8 + sizeof(time_t)];
        for (int i = 8; i < datalen; i++) {
            *datap++ = i;
        }

        int cc = datalen + 8;
        ping::encode_icmp(icp, IPV::icmp_query_type, ++transmitted, ident);
        icp->icmp_cksum = ping::icmp_cksum(outpack, cc);

        if (poll(&poll_send, 1, timeout) != 1) {
            if (!quiet) {
                std::cerr << "Send timeout" << std::endl;
            }
            return -1;
        }
        int code = sendto(sock, outpack, cc, 0, (sockaddr *) &sin, sizeof(sin_t));

        if (code >= 0 && code != cc) {
            if (!quiet) {
                std::cerr << code << '/' << cc << " chars been sent" << std::endl;
            }
            return -1;
        } else if (code < 0) {
            if (!quiet) perror("sendto");  // stderr
        }
        return code;
    }

    [[nodiscard]] int receive() {
        socklen_t fromlen = sizeof(sin_t);
        if (poll(&poll_recv, 1, timeout) != 1) {
            if (!quiet) {
                std::cerr << "Receive timeout" << std::endl;
            }
            return -1;
        }
        return recvfrom(sock, packet, MAXPACKET, 0, (sockaddr *) (&from), &fromlen);
    }

    [[nodiscard]] int process_packet(int len) {
        time_t cur_time = std::chrono::steady_clock::now();

        ip *pack_ip = (ip *) packet;
        int hlen = pack_ip->ip_hl << 2;

        static void *from_addr = ping::sin_get_addr((sin_t *) &from);
        static char pack_from[IPV::pack_len];
        inet_ntop(IPV::af, from_addr, pack_from, IPV::pack_len);

        if (len < hlen + ICMP_MINLEN) {
            if (!quiet) {
                std::cerr << "Too short packet (" << len << " bytes) from " << pack_from << std::endl;
            }
            return -1;
        }

        icmp_t *icp = (icmp_t *) (packet + hlen);
        if (icp->icmp_id != ident) return 0;

        int ttl = -1;
        socklen_t tmp = sizeof(ttl);
        getsockopt(sock, IPPROTO_IP, IP_TTL, &ttl, &tmp);

        uint64_t time_diff = 0;
        if (timing) {
            time_t send_time{};
            memcpy(&send_time, &icp->icmp_data[0], sizeof(time_t));
            auto rtt = std::chrono::duration_cast<std::chrono::milliseconds>(cur_time - send_time);
            time_diff = rtt.count();
            total_time += time_diff;
        }

        if (!quiet) {
            std::cout << len - hlen << " bytes"
                      << " from " << pack_from << ": "
                      << "icmp_seq=" << icp->icmp_seq
                      << " ttl=" << ttl;
            if (timing) {
                std::cout << " time=" << time_diff << "ms";
            }
            std::cout << std::endl;
        }
        return 0;
    }

    void print_stats() const {
        std::cout << "--- " << hostname << " ping stats ---" << std::endl;
        std::cout << transmitted << " packets transmitted, " << lost << " packets lost." << std::endl
                  << errors << " total errors";
        if (timing) {
            std::cout << ", " << total_time << "ms total time";
        }
        std::cout << std::endl;
    }
};

#endif //PING_PINGER_H