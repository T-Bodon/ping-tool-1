# What is it?

Please write a small Ping CLI application for Linux.
The CLI app should accept a hostname or an IP address as its argument, then send ICMP "echo requests" in a loop to the target while receiving "echo reply" messages.

- reports loss and RTT times for each sent message.
- supports IPv4 and IPv6
- allows to set TTL as an argument and report the corresponding "time exceeded” ICMP messages
- some additional features listed in the ping man page
