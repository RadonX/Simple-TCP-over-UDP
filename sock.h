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

#define SOCKADDR_LEN sizeof(struct sockaddr_in)
// socklen_t length = sizeof(struct sockaddr_in);



void setup_local_sockaddr(struct sockaddr_in &addr, int port)
{
    bzero((char *) &addr, SOCKADDR_LEN);
    addr.sin_family = AF_INET; //~~ server byte order
    addr.sin_addr.s_addr = INADDR_ANY; // automatically be filled with current host's IP address
    addr.sin_port = htons(port); // convert short integer value for port must be converted into network byte order
}

//  bind the socket to the current IP address on port
void bind_socket(int sockfd, int port)
{
    struct sockaddr_in addr;
    setup_local_sockaddr(addr, port);
        // though every time we bind a socket, the local addr is reconstructed, it is trivial
    if (bind(sockfd, (struct sockaddr *)&addr, SOCKADDR_LEN)<0)
        error("ERROR on binding");
}

//  bind the socket to the current IP address on port
void bind_socket6(int sockfd, int port)
{
    struct sockaddr_storage addr;
    /*
       fill addr structure using an IPv4/IPv6 address and
       fill addrlen before calling socket function
    */
    socklen_t addrlen;

//    setup_local_sockaddr(addr, port);
    // though every time we bind a socket, the local addr is reconstructed, it is trivial
    if (bind(sockfd, (struct sockaddr *)&addr, addrlen)<0)
        error("ERROR on binding");

}


//create_socket(arg[2], argv[3], SOCK_STREAM)
void set_sockaddr(struct sockaddr_storage &sockaddr, char *addr, char *port, int socktype)
{
    int status;

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = socktype;
    /* Setting AI_PASSIVE will give you a wildcard address if addr is NULL */
    hints.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV | AI_PASSIVE;

    if ((status = getaddrinfo(addr, port, &hints, &res)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s %s\n", gai_strerror(status), addr);
        return;
    }


    memcpy(&sockaddr, res->ai_addr, res->ai_addrlen);

    printf("done %d\n", res->ai_addrlen);

//    return socket(res->ai_family, res->ai_socktype, res->ai_protocol);
}

//    setup_host_sockaddr(recv_addr, argv[2], atoi(argv[3]) );
void setup_host_sockaddr(struct sockaddr_in &addr, char *hostname, int port )
{
    struct hostent *host;
    host = gethostbyname(hostname); // ipv4 only
    if (host == NULL) error("Unknown host");
    bzero((char *) &addr, sizeof(addr));
    addr.sin_family = AF_INET;
    bcopy((char *) host->h_addr, (char *) &addr.sin_addr.s_addr, host->h_length);
    addr.sin_port=htons(port);
}