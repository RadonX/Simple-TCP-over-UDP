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
    if (argc != 3) {
        printf("Usage: receiver <server> <port>\n");
        exit(1);
    }

    //  create a socket for UDP
    int sockfd;
    sockfd = socket(PF_INET, SOCK_DGRAM, 0); //~~ PF_INET6
    if (sockfd < 0) error("Opening socket");

    struct sockaddr_in serv_addr, from;
    socklen_t length = sizeof(struct sockaddr_in);

    struct hostent *host;
    host = gethostbyname(argv[1]);
    if (host == NULL) error("Unknown host");
    //  setup the serv_addr structure for use in bind call 
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family=AF_INET; //~~ AF_UNSPEC
    bcopy((char *) host->h_addr, (char *) &serv_addr.sin_addr.s_addr, host->h_length);
    serv_addr.sin_port=htons(atoi(argv[2]));
    
    int n;
    char buffer[256];
    printf("Please enter the message: ");
    bzero(buffer, 256);
    fgets(buffer, 255, stdin);
    n = sendto(sockfd, buffer, strlen(buffer) + 1, 0, 
            (const struct sockaddr *)&serv_addr, length);
    if (n < 0) error("ERROR sendto");
    // bzero(buffer, 256);
    n = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *) &from, &length);
    if (n < 0) error("ERROR recvfrom");
    printf("%s\n", buffer);

    close(sockfd);

    return 0;
}