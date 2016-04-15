#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>

#include "sock.h"


int main(int argc, char *argv[]){
    
    //  parse arguments
    // > receiver file.txt 20000 127.0.0.1 20001 logfile.txt
    if (argc != 6) {
        printf("Usage: %s <filename> <listening_port> <sender_IP> <sender_port> <log_filename>\n", argv[0]);
        exit(1);
    }
    int port = atoi(argv[2]);


    struct sockaddr_in send_addr;
    setup_host_sockaddr(send_addr, argv[3], atoi(argv[4]) );
    
    //  create a TCP socket for ack
    int sockfd_tcp = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd_tcp < 0) error("Opening TCP socket");
    bind_socket(sockfd_tcp, port);
    //  connect to sender
    if (connect(sockfd_tcp, (struct sockaddr *) &send_addr, sizeof(send_addr)) < 0)
        error("ERROR connecting");


    //  create a socket for UDP
    int sockfd_udp = socket(PF_INET, SOCK_DGRAM, 0); //~~ PF_INET6
    if (sockfd_udp < 0) error("Opening UDP socket");
    bind_socket(sockfd_udp, port);
    
    int n;
    char buffer[256];
    struct sockaddr_in from;
    socklen_t fromlen;

    //  receive data from UDP socket    
    while (true){

        // bzero(buffer, 256);
        n = recvfrom(sockfd_udp, buffer, sizeof(buffer), 0, (struct sockaddr *) &from, &fromlen);
        if (n < 0) error("ERROR recvfrom");
        printf("%s > from %s port %d\n", buffer, inet_ntoa(from.sin_addr), ntohs(from.sin_port));
        //~~ validate data src

        //~~ how to break when sockfd is closed
    }
   
    close(sockfd_tcp);
    close(sockfd_udp);

    return 0;
}