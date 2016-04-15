#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>


void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[]){
    if (argc != 4) {
        printf("Usage: receiver <server> <serv_port> <portno>\n");
        exit(1);
    }

    //  create a socket for UDP
    int sockfd;
    sockfd = socket(PF_INET, SOCK_DGRAM, 0); //~~ PF_INET6
    if (sockfd < 0) error("Opening socket");

    socklen_t length = sizeof(struct sockaddr_in);

    //  setup the host_addr structure for use in bind call 
    struct sockaddr_in cli_addr;
    bzero((char *) &cli_addr, length);
    cli_addr.sin_family = AF_INET; // server byte order
    cli_addr.sin_addr.s_addr = INADDR_ANY; // automatically be filled with current host's IP address
    cli_addr.sin_port = htons(atoi(argv[3])); // convert short integer value for port must be converted into network byte order
    //  bind the socket to the current IP address on port, portno
    if (bind(sockfd, (struct sockaddr *)&cli_addr, length)<0) 
        error("ERROR on binding");
        
    struct sockaddr_in serv_addr, from;

    //  setup the serv_addr structure 
    struct hostent *host;
    host = gethostbyname(argv[1]);
    if (host == NULL) error("Unknown host");
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family=AF_INET; //~~ AF_UNSPEC
    bcopy((char *) host->h_addr, (char *) &serv_addr.sin_addr.s_addr, host->h_length);
    serv_addr.sin_port=htons(atoi(argv[2]));
    
    
    int n;
    char buffer[256] = "hi, sender";
    
    n = sendto(sockfd, buffer, strlen(buffer) + 1, 0, 
            (const struct sockaddr *)&serv_addr, length);
    if (n < 0) error("ERROR sendto");
    
    while (true){

        // bzero(buffer, 256);
        n = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *) &from, &length);
        if (n < 0) error("ERROR recvfrom");
        printf("%s from port: %d\n", buffer, ntohs(from.sin_port));

        // n = sendto(sockfd, "receiver got U", 8, 0, 
        //         (const struct sockaddr *)&from, length);
        // if (n < 0) error("ERROR sendto");

    }
   
    close(sockfd);

    return 0;
}