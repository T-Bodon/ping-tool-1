#include <iostream>
#include <string>

#include "utils.h"
#include "ping_error.h"

// TODO:

// constructors with opts
// stats: timing, loss etc
// io context
// ipv4 ipv6 switcher

struct ping_caller {
    static const size_t MAXPACKET = 4096;
    char packet[MAXPACKET];

    sockaddr whereto{};
    sockaddr_in from;
    const char *hostname;

    int sock;
    int datalen = 64 - 8;
    int ntransmitted = 0;
    int ident;

    explicit ping_caller(const char *host) {
        sockaddr_in *sain = (sockaddr_in *) &whereto;
        bzero((char *) &whereto, sizeof(struct sockaddr));
        sain->sin_family = AF_INET;
        sain->sin_addr.s_addr = inet_addr(host);
        if (sain->sin_addr.s_addr != (unsigned) -1) {
            hostname = host;
        } else {
            hostent *hp = gethostbyname(host);
            if (hp) {
                sain->sin_family = hp->h_addrtype;
                bcopy(hp->h_addr, (caddr_t) &sain->sin_addr, hp->h_length);
                hostname = hp->h_name;
            } else {
                throw ping_error("Unknown host");
            }
        }
        protoent *proto = getprotobyname("icmp");
        if (proto == nullptr) {
            throw ping_error("Unknown protocol: icmp");
        }
        sock = socket(AF_INET, SOCK_RAW, proto->p_proto);
        if (sock < 0) {
            throw ping_error("Failed open socket");
        }
        ident = getpid() & 0xFFFF;
    }

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

        u_char *datap = &outpack[8 + sizeof(struct timeval)];
        int cc = datalen + 8;            /* skips ICMP portion */
        for (int i = 8; i < datalen; i++)    /* skip 8 for time */
            *datap++ = i;

        icp->icmp_cksum = ping::in_cksum(icp, cc);
        int code = sendto(sock, outpack, cc, 0, &whereto, sizeof(struct sockaddr));
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

int main() {
    ping_caller("google.com").ping(5);
    //ping_caller("etbenewnwnr.cur").ping(5);
    ping_caller("64.233.164.101").ping(5);
    //ping_caller("2a00:1450:4010:c07::64");
    return 0;
}