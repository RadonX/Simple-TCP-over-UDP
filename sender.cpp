#include "tcpstate.h"
#include "mytcp.h"
#include "sock.h"

#include "sharelib.h"

#define ACKBUF_LEN  400

struct mytcphdr tcp_hdr;
struct sockaddr_in recv_addr;
int sockfd_tcp, sockfd_udp;
unsigned char packet_buf[MYTCPHDR_LEN + TCP_DATA_SIZE]; // for packet sent to socket


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
    setsockopt(sockfd_udp, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int));
    bind_socket(sockfd_udp, srcport);


    //:: init window and buffer
    unsigned char buffer[TCP_DATA_SIZE]; // for data read from file
    set_tcphdr(tcp_hdr, srcport, ntohs(recv_addr.sin_port));

    init_tcpfsm();//~~

    FILE* fp = fopen(filename, "rb");
    if (fp == NULL)
        error("Can't open the file.\n");


    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(sockfd, &readfds);


    timeval timeout, zero_timeout;
    int initialWait = 5;
    zero_timeout.tv_sec = 0;
    zero_timeout.tv_usec = 1000;
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
    int pkt_ind = 0, n_ackpkt = 0;
    unsigned char ack_buf[ACKBUF_LEN];

    while (!fin){

        switch (event) {

            case READDATA:
                while (!eof){
                    //: add data to window if there is space and send it
                    ind = add_data_to_window(buffer, bufferlen, npkt, seq);
                    if ( ind != -1 ){
                        printf("add %d\n", seq);
                        send_packet(ind);
                        seq++; //~~ add_seq
                        npkt++;
                    } else{
                    //: otherwise wait for new event
                        break;
                    }
                    //: read new data from file
                    bufferlen = fread(buffer, sizeof(char), TCP_DATA_SIZE, fp);
                    eof = (bufferlen <= 0);
                }
                printf("READDATA upto %d\n", npkt);
                event = WAIT;//~~
                break;

            case ACK:

                event = WAIT;//~~

                if (pkt_ind >= n_ackpkt)
                    error("shouldn't happen when event == ACK");

                //: parse received packet
                printf("%d / %d\n", pkt_ind, n_ackpkt);
                unpack_tcphdr(ack_buf + pkt_ind, tmptcphdr);
                pkt_ind += MYTCPHDR_LEN; // len of ack pkt
                //: log tcp header
                log_timestamp(logbuffer);
                log_tcphdr(logbuffer+10, tmptcphdr);
                puts(logbuffer);

                if (tmptcphdr.th_flags & TH_ACK){
                    acknum = tmptcphdr.th_ack;
                } else break;

                if (acknum > 0)
                    ack_window(acknum-1);// get packet < acknum, currently seq == npkt
                printf("ack %d\n", acknum);
                // always try to read new data after each ACK
                if (!eof){
                    event = READDATA;
                    continue;// can skip WAIT state
                }

                break;

            case TIMEOUT:
                break;
            default:
                break;
        }

        //P64: change timeout

//        printf("wait\n");

        if (pkt_ind < n_ackpkt) {
            event = ACK;//~~ still ACK event
            continue;
        }

        //:: wait for new event (ack/timeout)
        printf("select()\n");
        // make sure sockfd in readfds before select()
        int ret_select = select(sockfd + 1, &readfds, NULL, NULL, &timeout);
        if (ret_select == -1)
            error("ERROR select()");
        else if (ret_select == 0) {
            event = TIMEOUT; //~~
            printf("timeout\n");
            continue;
        }


        if (FD_ISSET(sockfd, &readfds) ){ //~~ seems that after receiver sock closed, here always being true
            n_ackpkt = recv(sockfd, ack_buf, sizeof ack_buf, 0);
            pkt_ind = 0;
            // since multiple pkts can huddle, receiver should send only fixed-length pkts
            // otherwise need to provide `option` so as to differentiate tcp pkts
            event = ACK;
        } else
            error("`if` should be true or never be reached after select()");



        //fin = eof || ;

    }


    //: send an empty packet
    int n = sendto(sockfd_udp, packet_buf, MYTCPHDR_LEN, 0,
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
