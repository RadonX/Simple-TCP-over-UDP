#include "mytcp.h"
#include "sock.h"
#include "pack.h"

#include "sharelib.h"


int main(int argc, char *argv[]){
    
    //  parse arguments
    // > receiver file.txt 20000 127.0.0.1 20001 logfile.txt
    if (argc != 6) {
        printf("Usage: %s <filename> <listening_port> <sender_IP> <sender_port> <log_filename>\n", argv[0]);
        exit(1);
    }
    int port = atoi(argv[2]);
    char *filename = argv[1];


    unsigned char buffer[MYTCPHDR_LEN + BUFFER_SIZE];
    int n;

    struct sockaddr_in send_addr;
    setup_host_sockaddr(send_addr, argv[3], atoi(argv[4]) );

    //: create a TCP socket for ack
    int sockfd_tcp = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd_tcp < 0) error("Opening TCP socket");
    bind_socket(sockfd_tcp, port);
    //: connect to sender
    if (connect(sockfd_tcp, (struct sockaddr *) &send_addr, sizeof(send_addr)) < 0)
        error("ERROR connecting");
    n = read(sockfd_tcp, buffer, 255);
    if (n < 0) error("ERROR reading from socket_tcp");
    printf("Connected: %s\n", buffer);

    //: create a socket for UDP
    int sockfd_udp = socket(PF_INET, SOCK_DGRAM, 0); //~~ PF_INET6
    if (sockfd_udp < 0) error("Opening UDP socket");
    // set SO_REUSEADDR on a socket to true (1):
    int optval = 1;
    //setsockopt(sockfd_udp, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);
    bind_socket(sockfd_udp, port);
    
    struct sockaddr_in from;
    socklen_t fromlen;
    int offset;
    struct mytcphdr tcp_hdr;
    init_tcphdr(tcp_hdr);

    FILE* fp = fopen(filename, "wb");
    if (fp == NULL)
        error("Can't open the file.\n");


    //  receive data from UDP socket    
    while (true){

        // bzero(buffer, 256);
        n = recvfrom(sockfd_udp, buffer, sizeof(buffer), 0, (struct sockaddr *) &from, &fromlen);
        if (n < 0) error("ERROR recvfrom");
        if (n <= MYTCPHDR_LEN) break;

        //  :memcpy (tcp_hdr, buffer, n);
        unpack(buffer, "HHLLCCHHH", &tcp_hdr.th_sport, &tcp_hdr.th_dport,
               &tcp_hdr.th_seq, &tcp_hdr.th_ack, &offset,
               &tcp_hdr.th_flags, &tcp_hdr.th_win, &tcp_hdr.th_sum, &tcp_hdr.th_urp
        );
        tcp_hdr.th_off = offset >> 4;

        fwrite(buffer + MYTCPHDR_LEN, sizeof(char), sizeof(buffer) - MYTCPHDR_LEN, fp);

        printf(" %d %d", tcp_hdr.th_seq, n);

        printf("  > from %s port %d\n", inet_ntoa(from.sin_addr), ntohs(from.sin_port));
        //~~ validate data src

        //~~ how to break when sockfd is closed
    }

    fclose(fp);
    close(sockfd_tcp);
    close(sockfd_udp);

    return 0;
}