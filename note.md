
serv_addr.sin_family = AF_INET;
serv_addr.sin_addr.s_addr = INADDR_ANY;
serv_addr.sin_port = htons(portno);


int getaddrinfo(const char *node, const char *service, const struct addrinfo *host_info, struct addrinfo **res);




 tunnelbroker.net to tunnel IPv6 through IPv4
    Open up all ICMP in your Security Group to that IP address. I think it's a newer (time may be relative) thing where they ping your IP address before they configure your tunnel. They didn't used to quite some ago, and my tunnel is quite aged.

---


 you can drop out of order packets. You should note in your README if your implementation stores them (However, I don't think this is all that hard if you keep a separate buffer chain.)


 #### issues

 1. TCP packet size. MSS

 2. concurrency or select().

 3. we need to verify the checksum for the ACK packets on the sender side, and disregard ACKs with errors in the checksum?

 4. building a header with char* in C

 using the pack() and unpack() function. They take a series of numbers and parameters specifying each number's type (int, short, long) and return the bit format of the bits.

 Here is an example command to 'pack' the header into it's bit format:
 bitHeader = pack( 'HHLLBBHHH', source, dest, seq, ack_seq, offset, unusedFlags, windowSize, check, urg_ptr )
     'HHLLBBHHH' is the format of the bits. H stands for short int (2 bytes), L stands for int (4 bytes) B is binary (1 byte)

 Now you can just append the packet payload to this bit header, send it through the link, get the packet on the other side and remove the first 32 characters from the packet (think header = packet[:32]) unpack them and now on the other side you have a tuple of all your packet header values!

 5. UDP will automatically tell you how much data you have, so there's no need for zero bytes. (This would also not allow files that contain zero bytes, such as binary files.)

 6. use an initial timeout length of 3s, and then commence using RTT and devRTT after two RTT measurements.