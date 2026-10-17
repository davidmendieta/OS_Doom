# Notes on making the parser

## Questions

### How do we recieve packets?
When the user calls send(), if the send() call is trying to send data over the network,
the syscall will call our parser function.
The parser funciton will ingest the raw bytes, and from there we can do our logic.

### How should we parse the packet we recieve?
Refer to the top level protocol below. Also, we can spawn a thread for each one
we are trying to parse.
It might be wasteful to try to parse when the packet isn't actually for us, so 
we can hold off on spawning the threads before we verify the intented reciever 
of the packet.

### How should we build packets?
Refer to top level protocol.

### How do we respond?
We need to find out how to "send" to the NIC. 
sendto() syscall?


-------


# TOP LEVEL - PROTOCOL


IPv4 Header (20 bytes without options)
---------------------------------------
Field                   Size (bytes)
-------------------------------
Version + IHL           1
Type of Service         1
Total Length            2
Identification          2
Flags + Fragment Offset 2
TTL                     1
Protocol                1
Header Checksum         2
Source IP               4
Destination IP          4

UDP Header (8 bytes)
-------------------------------
Field                   Size (bytes)
-------------------------------
Source Port             2
Destination Port        2
Length                  2
Checksum                2

------

First 20 bytes = IP header

Next 8 bytes = UDP header

Remaining = payload
