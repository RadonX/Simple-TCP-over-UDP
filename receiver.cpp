#include "tcpstate.h"
#include "mytcp.h"
#include "sock.h"

#include "sharelib.h"


struct mytcphdr tcp_hdr;
unsigned char packet_buf[MYTCPHDR_LEN];
int sockfd_tcp;


void write_log(struct mytcphdr &tcp_hdr )
{
    log_timestamp(logbuffer);
    log_tcphdr(logbuffer+10, tcp_hdr);
    puts(logbuffer);
}

void send_ack(int ack){
    set_tcphdr_ack(tcp_hdr, ack);
    pack_tcphdr(packet_buf, tcp_hdr);
    send(sockfd_tcp, packet_buf, sizeof(packet_buf), 0);
}

int main(int argc, char *argv[]){
    
    //  parse arguments
    // > receiver file.gz 20000 127.0.0.1 20001 logfile.txt
    if (argc != 6) {
        printf("Usage: %s <filename> <listening_port> <sender_IP> <sender_port> <log_filename>\n", argv[0]);
        exit(1);
    }
    int srcport = atoi(argv[2]);
    char *filename = argv[1];


    unsigned char buffer[MYTCPHDR_LEN + TCP_DATA_SIZE];
    int n;

    struct sockaddr_in send_addr;
    setup_host_sockaddr(send_addr, argv[3], atoi(argv[4]) );

    //: create a TCP socket for ack
    sockfd_tcp = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd_tcp < 0) error("Opening TCP socket");
    int optval = 1;
    setsockopt(sockfd_tcp, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);
    bind_socket(sockfd_tcp, srcport);
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
    setsockopt(sockfd_udp, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);
    bind_socket(sockfd_udp, srcport);
    
    struct sockaddr_in from;
    socklen_t fromlen;
    struct mytcphdr tmptcphdr;
    set_tcphdr(tcp_hdr, srcport, ntohs(send_addr.sin_port));

    FILE* fp = fopen(filename, "wb");
    if (fp == NULL)
        error("Can't open the file.\n");

    tcp_seq expseq = 0;

    //  receive data from UDP socket    
    while (true){

        // bzero(buffer, 256);
        n = recvfrom(sockfd_udp, buffer, sizeof(buffer), 0, (struct sockaddr *) &from, &fromlen);
        if (n < MYTCPHDR_LEN) error("ERROR recvfrom");
        if (n != MYTCPHDR_LEN + TCP_DATA_SIZE) {
            printf("++++++\n%d\n++++++\n", n);
            if (n == MYTCPHDR_LEN) break;
        }

        unpack_tcphdr(buffer, tmptcphdr);
        write_log(tmptcphdr);

        if (expseq == tmptcphdr.th_seq){
            fwrite(buffer + MYTCPHDR_LEN, sizeof(char), n - MYTCPHDR_LEN, fp);//~~ sizeof(buffer)??
            expseq++;
        } // else: drop out-of-order pkt
        send_ack(expseq);

        write_log(tcp_hdr);

//        printf("  >> from %s port %d\n", inet_ntoa(from.sin_addr), ntohs(from.sin_port));


        //~~ validate data src

        //~~ how to break when sockfd is closed
    }

    fclose(fp);
    printf("Delivery completed successfully\n");

    close(sockfd_tcp);
    close(sockfd_udp);

    return 0;
}