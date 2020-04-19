#include <iostream>
#include <string>

#include "pinger.h"

int main() {
    for (std::string const &addr : {"vk.com", "google.com", "unrealsiteurl.test", "64.233.164.101",
                                    "2a00:1450:4010:c07::64"}) {
        try {
            std::cout << "IPv4 ping:" << std::endl;
            pinger<ping::tag_ipv4> a(addr);
            a.ping(3);
            a.print_stats();
        } catch (ping_error const &e) {
            std::cerr << e.what() << std::endl;
        }

        // I couldnt test it good, because my machine isn't ipv6 friendly
        try {
            std::cout << "IPv6 ping:" << std::endl;
            pinger<ping::tag_ipv6> a(addr);
            a.ping(3);
            a.print_stats();
        } catch (ping_error const &e) {
            std::cerr << e.what() << std::endl;
        }
    }
    return 0;
}
