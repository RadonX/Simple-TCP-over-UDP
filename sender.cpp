#include "mytcp.h"
#include "sock.h"
#include "pack.h"

#include "sharelib.h"
#include "window.h"



struct mytcphdr tcp_hdr;
unsigned char packet_buf[MYTCPHDR_LEN + TCP_DATA_SIZE];
struct sockaddr_in recv_addr;
int sockfd_tcp, sockfd_udp;


void send_packet(int ind){
    tcp_hdr.th_seq = window[ind].seq;
    pack(packet_buf, "HHLLCCHHH", tcp_hdr.th_sport, tcp_hdr.th_dport,
         tcp_hdr.th_seq, tcp_hdr.th_ack, (tcp_hdr.th_off << 4) + tcp_hdr.th_x2,
         tcp_hdr.th_flags, tcp_hdr.th_win, tcp_hdr.th_sum, tcp_hdr.th_urp);

    memcpy(packet_buf + MYTCPHDR_LEN, window[ind].data, window[ind].size);

    //  send data via UDP socket to recv_addr
    int n = sendto(sockfd_udp, packet_buf, MYTCPHDR_LEN + window[ind].size, 0,
               (const struct sockaddr *)&recv_addr, SOCKADDR_LEN);
    if (n < 0) error("ERROR sendto");

    window[ind].state = 0;
}


int main(int argc, char *argv[])
{

    //:: parse arguments
    // > sender newudpl-1.4.tar.gz 127.0.0.1 20000 20001 logfile.txt 3
    if (argc != 7) {
        fprintf(stderr, "Usage: %s <filename> <remote_IP> <remote_port> <ack_port_num> <log_filename> <window_size>\n", argv[0]);
        exit(0);
    }
    int srcport = atoi(argv[4]);
    char *filename = argv[1];
    char *logfile = argv[5];
    setup_host_sockaddr(recv_addr, argv[2], atoi(argv[3]) );
    cwnd = atoi(argv[6]);
    if (cwnd > MAX_CWND) {
        fprintf(stderr, "Max window size supported is %d\n", MAX_CWND);
        exit(0);
    }



    //  create a TCP socket for ack
    sockfd_tcp = socket(PF_INET, SOCK_STREAM, 0);
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
        send(sockfd, "Hello, receiver!\n", 20, 0);
        break;
        //?? only when there is `fread()` below, from != recv
        //?? from/recv seem to refer to the same receiver, being of address/protocol family
        /*
        if (from.sin_addr.s_addr == recv_addr.sin_addr.s_addr && from.sin_port == recv_addr.sin_port){
            break;
        } else{
            printf("Goodbye~\n");
            close(sockfd);
        }
         */
    }

    
    //  create a UDP socket
    sockfd_udp = socket(PF_INET, SOCK_DGRAM, 0); //~~ PF_INET6
    if (sockfd_udp < 0) error("Opening UDP socket");
    bind_socket(sockfd_udp, srcport);


    fd_set readfds, writefds;
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    FD_SET(sockfd_tcp, &readfds);
    FD_SET(sockfd_udp, &writefds);
//    int maxfd = max(sockfd_tcp, sockfd_udp);
//    select(sockfd_tcp+1, &readfds, NULL, NULL, &tv);


    timeval timeout;
    int initialWait = 3;
    timeout.tv_usec = 0;
    timeout.tv_sec = initialWait ;//* 2;

    //: init window and buffer
    int n;
    unsigned char buffer[TCP_DATA_SIZE];
    unsigned char hdr_buf[24];
    init_tcpfsm();
    set_tcphdr(tcp_hdr, srcport, ntohs(recv_addr.sin_port));


    FILE* fp = fopen(filename, "rb");
    if (fp == NULL)
        error("Can't open the file.\n");

    int bufferlen;

    //bzero(buffer, 256);


    //:: init TCP state machine
    int seq = 0, npkt = 0, ind;
    bufferlen = fread(buffer, sizeof(char), TCP_DATA_SIZE, fp);
    bool eof = (bufferlen <= 0);



    while (!eof){

        while (!eof){
            //: add data to window if there is space and send it
            if ( (ind = add_data_to_window(buffer, bufferlen, npkt, seq)) != -1 ){
                send_packet(ind);
                seq++; //~~ add_seq
                npkt++;
            } else break;
            //: read new data from file
            bufferlen = fread(buffer, sizeof(char), TCP_DATA_SIZE, fp);
            eof = (bufferlen <= 0);
        }

        sleep(1);
        printf("some packets sent\n");

    }


    //: send an empty packet
    n = sendto(sockfd_udp, packet_buf, MYTCPHDR_LEN, 0,
               (const struct sockaddr *)&recv_addr, SOCKADDR_LEN);
    if (n < 0) error("ERROR sendto");

    printf("Delivery completed successfully\n");
    printf("Total bytes sent = \n");
    printf("Segments sent = %d\n", npkt);
    printf("Segments retransmitted = 0 %%\n");

    fclose(fp);

    //::  FIN

    //~~  close socket

    close(sockfd_tcp);
    close(sockfd_udp);

    return 0;
 }
