


pthread_mutex_t syncWindowAccess = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_init(&syncWindowAccess, NULL);

pack/unpack (diff machines)!!

 memcpy:  assumes that your two machines use the same byte order, and have been careful to take account of structure padding.


	printf("'%c' %hhu %u %ld \"%s\" %f\n", magic, ps2, monkeycount,
			altitude, s2, absurdityfactor);


 (There's no place to put the missing sequence numbers in the standard header, for example.) Reading the relevant RFC (RFC 793)

P21

struct sockaddr_in6 ip6addr;
int s;
ip6addr.sin6_family = AF_INET6;
ip6addr.sin6_port = htons(4950);
inet_pton(AF_INET6, "2001:db8:8714:3a90::12", &ip6addr.sin6_addr);
s = socket(PF_INET6, SOCK_STREAM, 0);
bind(s, (struct sockaddr*)&ip6addr, sizeof ip6addr);

char ip6[INET6_ADDRSTRLEN]; // space to hold the IPv6 string
struct sockaddr_in6 sa6;    // pretend this is loaded with something
inet_ntop(AF_INET6, &(sa6.sin6_addr), ip6, INET6_ADDRSTRLEN);
printf("The address is: %s\n", ip6);

cpp -> c

int getaddrinfo(const char *node, const char *service, const struct addrinfo *host_info, struct addrinfo **res);
> getaddrinfo("www.example.com", "3490", &hints, &res);

 the getaddrinfo() function to handle this. By the looks of it, with getaddrinfo you can handle both together by unspecifying the family. Googling getaddrinfo() will be of heaps of help.


 tunnelbroker.net to tunnel IPv6 through IPv4
    Open up all ICMP in your Security Group to that IP address. I think it's a newer (time may be relative) thing where they ping your IP address before they configure your tunnel. They didn't used to quite some ago, and my tunnel is quite aged.



because this is a homework where we want to play with tcp header, the udp/tcp sockets are tunnels that we don't make any modifications.

ERROR connecting: Address already in use



P64: change timeout


http://serverfault.com/questions/204893/aws-ec2-and-build-essential


bzero, memset

#### resources

1. [Beej's Guide to Network Programming](http://beej.us/guide/bgnet/)

    Scan to understand the basic of network programming.


2. [C Language Examples of IPv4 and IPv6 Raw Sockets for Linux](http://www.pdbuchan.com/rawsock/rawsock.html)

    Code snippets.


 1. [networking - parse IP and TCP header (especially common tcp header options)of packets captured by libpcap - Stack Overflow](http://stackoverflow.com/questions/16519846)


https://en.wikipedia.org/wiki/Transmission_Control_Protocol


[Debugging in CLion | CLion Blog](http://blog.jetbrains.com/clion/2015/05/debug-clion/)

    Don't use print for debugging, especially for complicated programs. It will save you time and discover potential bugs.