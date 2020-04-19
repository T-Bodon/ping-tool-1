PLEASE, BE SURE YOU RUN IT WITH SUDO

EXAMPLE: sudo ./pinger -c 10 google.com

Pinger class provides opportunities to:

- ping in infinite loop
- ping given number of times (-c in Linux ping)

- report loss and rtt times for each message
- report loss and rtt times total

- support IPv4 and IPv6 (IPv6 not tested due to my machine isn't IPv6 friendly). -X option.
- set TTL (-t in Linux ping)
- also: -r (SO_DONTROUTE), -d (SO_DEBUG), -W (timeout), -s (packet size), -q (quiet) from Linux ping
