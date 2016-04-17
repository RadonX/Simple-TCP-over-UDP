#include "mytcp.h"
#include "sock.h"
#include "pack.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h> //atoi()
#include <unistd.h> //close()
#include <arpa/inet.h> //inet_ntoa()
//#include <sys/types.h>
//#include <sys/socket.h>
//#include <netinet/in.h>



int main(int argc, char *argv[])
{

    //  parse arguments
    // > sender file.txt 127.0.0.1 20000 20001 logfile.txt 1152
    if (argc != 7) {
        fprintf(stderr, "Usage: %s <filename> <remote_IP> <remote_port> <ack_port_num> <log_filename> <window_size>\n", argv[0]);
        exit(0);
    }
    int srcport = atoi(argv[4]);

    char *filename;
    // filename = argv[1];

    struct sockaddr_in recv_addr;  
    setup_host_sockaddr(recv_addr, argv[2], atoi(argv[3]) );


    //  create a TCP socket for ack
    int sockfd_tcp = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd_tcp < 0) error("Opening TCP socket");
    bind_socket(sockfd_tcp, srcport);
    listen(sockfd_tcp, 5);

    struct sockaddr_in from;
    socklen_t fromlen;

    //  wait for notification from the remote host
    while (true){
        int sockfd = accept(sockfd_tcp, (struct sockaddr *) &from, &fromlen);
        if (sockfd < 0) error("ERROR on accept");
        printf("got connection from %s port %d\n", inet_ntoa(from.sin_addr), ntohs(from.sin_port));
        if (from.sin_addr.s_addr == recv_addr.sin_addr.s_addr && from.sin_port == recv_addr.sin_port){
            break;
        } else{
            printf("Goodbye~\n");
            close(sockfd);
        }
    }

    
    //  create a UDP socket
    int sockfd_udp = socket(PF_INET, SOCK_DGRAM, 0); //~~ PF_INET6
    if (sockfd_udp < 0) error("Opening UDP socket");
    bind_socket(sockfd_udp, srcport);

    
    int n;
    char buffer[256] = "this is data 0"; // len == 14
    struct mytcphdr tcp_hdr;
    int tcp_hdr_size;
    unsigned char hdr_buf[24];

    int seq = 0;

    printf("%d", htons(0));

    //  send data via UDP socket to recv_addr   
    while (true){
        buffer[13] = seq + '0';
        /*
        printf("Please enter the message: ");
        bzero(buffer, 256);
        fgets(buffer, 255, stdin);
         */

        set_tcphdr(tcp_hdr, srcport, ntohs(recv_addr.sin_port), seq, 5 );
        tcp_hdr_size = pack(hdr_buf, "HHLLCCHHH", tcp_hdr.th_sport, tcp_hdr.th_dport,
                tcp_hdr.th_seq, tcp_hdr.th_ack, (tcp_hdr.th_off << 4) + tcp_hdr.th_x2,
                tcp_hdr.th_flags, tcp_hdr.th_win, tcp_hdr.th_sum, tcp_hdr.th_urp
        );


        sleep(1);

        n = sendto(sockfd_udp, hdr_buf, tcp_hdr_size, 0,
                (const struct sockaddr *)&recv_addr, SOCKADDR_LEN);
        if (n < 0) error("ERROR sendto");

        seq++;
        if (seq == 10) seq = 0;
    }
    
    //~~  close socket
     
    return 0;
 }
