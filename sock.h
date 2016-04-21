#include <netdb.h> // host
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h> //sockaddr_in

void error(const char *msg)
{
    perror(msg);
    exit(0);
}


int set_sockaddr(struct sockaddr_storage &sockaddr, socklen_t &addrlen, char *addr, char *port, int aifamily = AF_UNSPEC)
{
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = aifamily;
//    hints.ai_socktype = socktype;
    /* Setting AI_PASSIVE will give you a wildcard address if addr is NULL */
    hints.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV | AI_PASSIVE;

    int status;
    if ((status = getaddrinfo(addr, port, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s %s\n", gai_strerror(status), addr);
        return -1;
    }

    addrlen = res->ai_addrlen;
    memcpy(&sockaddr, res->ai_addr, addrlen);
    aifamily = res->ai_family;
    freeaddrinfo(res); // socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    return aifamily;
}

//  bind the socket to the current IP address on port
int create_and_bind_socket(const struct sockaddr_storage &sockaddr, socklen_t addrlen, int aifamily, int socktype)
{
    int sockfd = socket(aifamily, socktype, 0);
    if (sockfd < 0) error("Opening socket");

    //: set SO_REUSEADDR on a socket to true (1)
    //  lose the pesky "address already in use" error message
    int optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int));

    if (bind(sockfd, (struct sockaddr *)&sockaddr, addrlen)<0)
        error("ERROR on binding");

    return sockfd;
}
