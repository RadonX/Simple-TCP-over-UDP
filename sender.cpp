#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>

void error(const char *msg)
{
    perror(msg);
    exit(0);
}


int main(int argc, char *argv[])
{

    if (argc < 2) {
        fprintf(stderr, "Usage: sender <port> \n");
        exit(0);
    }

    //  create a socket for UDP
    int sockfd;
    sockfd = socket(PF_INET, SOCK_DGRAM, 0); //~~ PF_INET6
    if (sockfd < 0) error("Opening socket");

    struct sockaddr_in serv_addr, from;
    socklen_t length = sizeof(struct sockaddr_in);
   
    //  setup the host_addr structure for use in bind call 
    bzero((char *) &serv_addr, length);
    serv_addr.sin_family = AF_INET; // server byte order
    serv_addr.sin_addr.s_addr = INADDR_ANY; // automatically be filled with current host's IP address
    serv_addr.sin_port = htons(atoi(argv[1])); // convert short integer value for port must be converted into network byte order

    //  bind the socket to the current IP address on port, portno
    if (bind(sockfd, (struct sockaddr *)&serv_addr, length)<0) 
        error("ERROR on binding");

    int n;
    char buffer[256];
    
    n = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *) &from, &length);
    if (n < 0) error("ERROR recvfrom");
    printf("%s from port: %d\n", buffer, ntohs(from.sin_port));
        
    while (true){
        printf("Please enter the message: ");
        bzero(buffer, 256);
        fgets(buffer, 255, stdin);
        n = sendto(sockfd, buffer, strlen(buffer) + 1, 0, 
        (const struct sockaddr *)&from, length);
        if (n < 0) error("ERROR sendto");


        
    }
    
     
    return 0;
 }
