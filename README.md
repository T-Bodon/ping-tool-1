# What is it?

Small Ping CLI application for Linux.
Accepts a hostname or an IP address as its argument, then sends ICMP "echo requests" in a loop to the target while receiving "echo reply" messages.

- reports loss and RTT times for each sent message.
- supports IPv4 and IPv6
- allows to set TTL as an argument and report the corresponding "time exceeded” ICMP messages
- some additional features listed in the ping man page
