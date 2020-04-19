#include <iostream>
#include <string>

#include "pinger.h"

void test() {
    for (std::string const &addr : {"vk.com", "google.com", "unrealsiteurl.test", "64.233.164.101",
                                    "2a00:1450:4010:c07::64"}) {
        try {
            std::cout << "IPv4 ping:" << std::endl;
            pinger<ping::tag_ipv4> a(addr);
            a.ping(30);
        } catch (ping_error const &e) {
            std::cerr << e.what() << std::endl;
        }

        // I couldnt test it good, because my machine isn't ipv6 friendly
        /*try {
            std::cout << "IPv6 ping:" << std::endl;
            pinger<ping::tag_ipv6> a(addr);
            a.ping(30);
        } catch (ping_error const &e) {
            std::cerr << e.what() << std::endl;
        }*/
    }
}

int main(int argc, char *argv[]) {
    const char * usage = "Usage: ping [-rdq] [-c count] [-s packetsize] [-t ttl] [-W timeout] [-X 4 or 6] destination\n";
    argc--, argv++;

    char *host;
    int times = -1;
    int timeout = 1000;
    int datalen = 56;
    int ttl = -1;
    int ipv = 4;
    bool quiet = false;
    bool debug = false;
    bool dontroute = false;

    while (argc > 1 && *argv[0] == '-') {
        switch (*++argv[0]) {
            case 'c': {
                times = std::stoi(argv[1]);
                argc--, argv++;
                break;
            }
            case 's': {
                datalen = std::stoi(argv[1]);
                argc--, argv++;
                break;
            }
            case 't': {
                ttl = std::stoi(argv[1]);
                argc--, argv++;
                break;
            }
            case 'W': {
                timeout = std::stoi(argv[1]);
                argc--, argv++;
                break;
            }
            case 'X': {
                ipv = std::stoi(argv[1]);
                argc--, argv++;
                break;
            }
            default: {
                while (*argv[0]) {
                    switch (*argv[0]) {
                        case 'r':
                            dontroute = true;
                            break;
                        case 'd':
                            debug = true;
                            break;
                        case 'q':
                            quiet = true;
                            break;
                        default: {
                            std::cerr << "Invalid args";
                            return argc;
                        }
                    }
                    ++argv[0];
                }
            }
        }
        argc--, argv++;
    }
    if (argc != 1 || (ipv != 4 && ipv != 6)) {
        std::cerr << usage;
        return argc;
    }
    host = argv[0];

    /* std::cout << host << " " << timeout << " " << datalen << " " << ttl << " " << ipv << " " << quiet << " " << debug
              << " " << dontroute << std::endl; */

    if (ipv == 4) {
        pinger<ping::tag_ipv4>(host, timeout, datalen, ttl, quiet, debug, dontroute).ping(times);
    } else {
        pinger<ping::tag_ipv6>(host, timeout, datalen, ttl, quiet, debug, dontroute).ping(times);
    }
    return 0;
}
