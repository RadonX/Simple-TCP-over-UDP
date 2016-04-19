#include "tcpstate.h"
#include "mytcp.h"
#include "sock.h"

#include "sharelib.h"



struct mytcphdr tcp_hdr;
unsigned char packet_buf[MYTCPHDR_LEN + TCP_DATA_SIZE]; // for data send to/recv from socket
struct sockaddr_in recv_addr;
int sockfd_tcp, sockfd_udp;


void send_packet(int ind){
    tcp_hdr.th_seq = window[ind].seq;
    pack_tcphdr(packet_buf, tcp_hdr);
    memcpy(packet_buf + MYTCPHDR_LEN, window[ind].data, window[ind].size);

    //  send data via UDP socket to recv_addr
    int n = sendto(sockfd_udp, packet_buf, MYTCPHDR_LEN + window[ind].size, 0,
               (const struct sockaddr *)&recv_addr, SOCKADDR_LEN);
    if (n < 0) error("ERROR sendto");

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



    //:: create a TCP socket for ack
    sockfd_tcp = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd_tcp < 0) error("Opening TCP socket");
    //  lose the pesky "address already in use" error message
    int optval = 1;
    setsockopt(sockfd_tcp, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int));
    bind_socket(sockfd_tcp, srcport);
    listen(sockfd_tcp, 5);
    int sockfd;


    //: wait for notification from the remote host
    struct sockaddr_in from;
    socklen_t fromlen;
    while (true){
        sockfd = accept(sockfd_tcp, (struct sockaddr *) &from, &fromlen);
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

    
    //:: create a UDP socket
    sockfd_udp = socket(PF_INET, SOCK_DGRAM, 0); //~~ PF_INET6
    if (sockfd_udp < 0) error("Opening UDP socket");
    bind_socket(sockfd_udp, srcport);


    //:: init window and buffer
    int n;
    unsigned char buffer[TCP_DATA_SIZE]; // for data read from file
    set_tcphdr(tcp_hdr, srcport, ntohs(recv_addr.sin_port));

    init_tcpfsm();//~~

    FILE* fp = fopen(filename, "rb");
    if (fp == NULL)
        error("Can't open the file.\n");



    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(sockfd, &readfds);


    timeval timeout;
    int initialWait = 5;
    timeout.tv_usec = 0;
    timeout.tv_sec = initialWait ;//* 2;

    //:: TCP state machine

    event = READDATA;
    int bufferlen;
    int ind, acknum;
    bufferlen = fread(buffer, sizeof(char), TCP_DATA_SIZE, fp);
    int seq = 0, npkt = 0;
    bool eof = (bufferlen <= 0), fin = false;
    struct mytcphdr tmptcphdr;

    while (!fin){

        switch (event) {

            case READDATA:
                while (!eof){
                    printf("READDATA");
                    //: add data to window if there is space and send it
                    if ( (ind = add_data_to_window(buffer, bufferlen, npkt, seq)) != -1 ){
                        printf("%d\n", npkt);
                        send_packet(ind);
                        seq++; //~~ add_seq
                        npkt++;
                    } else{
                        printf("\n");
                        break;
                    }
                    //: read new data from file
                    bufferlen = fread(buffer, sizeof(char), TCP_DATA_SIZE, fp);
                    eof = (bufferlen <= 0);
                }
                break;

            case ACK:
                ack_window(acknum);
                printf("ack%d\n", acknum);
                event = READDATA;
                continue;
                //break;
            case TIMEOUT:
                break;
        }

        printf("wait\n");


        //P64
        int ret_select = select(sockfd+1, &readfds, NULL, NULL, &timeout);
        if (ret_select == -1)
            error("ERROR select");
        else if (ret_select == 0){
            event = TIMEOUT; //~~
            printf("timeout\n");
            event = READDATA;
        }
        else{
            if (FD_ISSET(sockfd, &readfds) ){
                n = recv(sockfd, packet_buf, sizeof packet_buf, 0);
                unpack_tcphdr(packet_buf, tmptcphdr);
                if (tmptcphdr.th_flags & TH_ACK){
                    event = ACK;
                    acknum = tmptcphdr.th_ack;
                }
            }
        }

        //fin = eof || ;

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
