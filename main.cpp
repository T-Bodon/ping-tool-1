#include <iostream>
#include <string>

#include "pinger.h"

int main() {
    for (std::string const &addr : {"google.com", "unrealsiteurl.test", "64.233.164.101", "2a00:1450:4010:c07::64"}) {
        try {
            std::cout << "IPv4 ping:" << std::endl;
            pinger(addr).ping(2);
        } catch (ping_error const &e) {
            std::cerr << e.what() << std::endl;
        }


        try {
            std::cout << "IPv6 ping:" << std::endl;
            pinger<ping::tag_ipv6>(addr).ping(2);
        } catch (ping_error const &e) {
            std::cerr << e.what() << std::endl;
        }
    }
    return 0;
}