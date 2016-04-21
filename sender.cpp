#include "mytcp.h"
#include "sock.h"
#include "tcpstate.h"

#include "sharelib.h"

#define ACKBUF_LEN  400
//#define DEBUG


struct mytcphdr tcp_hdr;
struct sockaddr_storage recv_addr;
socklen_t recv_addrlen;
int sockfd_tcp, sockfd_udp;
unsigned char packet_buf[MYTCPHDR_LEN + TCP_DATA_SIZE ]; // for packet sent to socket
int nbytesent = 0, nseg = 0;
FILE *fp_log;

void write_log(struct mytcphdr &tcp_hdr )
{
    log_timestamp(logbuffer);
    log_tcphdr(logbuffer+TIMESTAMP_LEN, tcp_hdr);
    fprintf(fp_log, logbuffer);
    // log RTT
    fprintf(fp_log, ", RTT = %dms\n", rtt.srtt);
}

void send_packet(int ind){
    tcp_hdr.th_seq = window[ind].seq;
    pack_tcphdr(packet_buf, tcp_hdr);
    memcpy(packet_buf + MYTCPHDR_LEN, window[ind].data, window[ind].size);
    int packet_len = MYTCPHDR_LEN + window[ind].size;
    //: set checksum
    unsigned short checksum = calc_checksum(packet_buf, packet_len) ;//~~ htons
    memcpy(packet_buf + CHECKSUM_IND, &checksum, sizeof(checksum));
    //  send data via UDP socket to recv_addr
    int n = sendto(sockfd_udp, packet_buf, packet_len, 0,
                   (const struct sockaddr *)&recv_addr, recv_addrlen);
    nbytesent += packet_len; nseg++;
    if (n < 0) error("ERROR sendto");
    sent_from_window(ind);
    write_log(tcp_hdr);

}

void set_FIN_pkt(int seq)
{
    tcp_hdr.th_seq = seq;
    tcp_hdr.th_flags = TH_FIN;
    pack_tcphdr(packet_buf, tcp_hdr);
    //: set checksum
    unsigned short checksum = calc_checksum(packet_buf, MYTCPHDR_LEN) ;//~~ htons
    memcpy(packet_buf + CHECKSUM_IND, &checksum, sizeof(checksum));
}

void send_FIN_pkt()
{
    //:: try to send the FIN packet
    int n = sendto(sockfd_udp, packet_buf, MYTCPHDR_LEN, 0,
                   (const struct sockaddr *)&recv_addr, recv_addrlen);
    nbytesent += MYTCPHDR_LEN; nseg++;
    if (n < 0) error("ERROR sendto");
    write_log(tcp_hdr);
}


int main(int argc, char *argv[])
{

    //:: parse arguments
    // > sender newudpl-1.4.tar.gz 127.0.0.1 20000 20001 logfile.txt 3
    if (argc != 7) {
        fprintf(stderr, "Usage: %s <filename> <remote_IP> <remote_port> <ack_port_num> <log_filename> <window_size>\n", argv[0]);
        exit(0);
    }
    int srcport = atoi(argv[4]), dstport = atoi(argv[3]);
    char *filename = argv[1];
    char *logfile = argv[5];
    cwnd = atoi(argv[6]);
    if (cwnd > MAX_CWND) {
        fprintf(stderr, "Max window size supported is %d\n", MAX_CWND);
        exit(0);
    }
    //: set remote & local sockaddr
    socklen_t send_addrlen;
    struct sockaddr_storage send_addr;
    int aifamily = set_sockaddr(recv_addr, recv_addrlen, argv[2], argv[3]);
    set_sockaddr(send_addr, send_addrlen, NULL, argv[4], aifamily);

    //:: create a TCP socket for ack
    sockfd_tcp = create_and_bind_socket(send_addr, send_addrlen, aifamily, SOCK_STREAM);
    listen(sockfd_tcp, 5);
    int sockfd;
    //: wait for notification from the remote host
    struct sockaddr_storage from;
    socklen_t fromlen;
    while (true){
        sockfd = accept(sockfd_tcp, (struct sockaddr *) &from, &fromlen);
        if (sockfd < 0) error("ERROR on accept");
        //printf("got connection from %s port %d\n\n", inet_ntoa(from.sin_addr), ntohs(from.sin_port)); // ipv4 only
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
    sockfd_udp = create_and_bind_socket(send_addr, send_addrlen, aifamily, SOCK_DGRAM);
    printf("%d\n", getsockname(sockfd, (struct sockaddr *)&from, &fromlen) );

    //:: open files
    FILE* fp = fopen(filename, "rb");
    if (fp == NULL)
        error("Can't open the file.\n");
    fp_log = open_logfile(logfile);
    if (fp_log == NULL)
        error("Can't open the log file.\n");

    //:: init window, buffer, and tcp header
    init_tcpfsm(); // TCP state machine
    unsigned char buffer[TCP_DATA_SIZE]; // for data read from file
    set_tcphdr(tcp_hdr, srcport, dstport, cwnd);
    //:: init readfds, rtt
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(sockfd, &readfds);
    init_rtt();
    timeval &timeout = rtt.tv;

    int bufferlen;
    int ind, acknum;
    int seq = 0, npkt = 0;
    int done = 0;
    bool eof;
    struct mytcphdr tmptcphdr;
    int pkt_ind = 0, n_ackpkt = 0, finnum;
    unsigned char ack_buf[ACKBUF_LEN];

    bufferlen = fread(buffer, sizeof(char), TCP_DATA_SIZE, fp);
    eof = (bufferlen <= 0);
    while (done != 2 && !tcpfsm.transmax){
        if (eof && (tcpfsm.sendbase == npkt) && done == 0)  {
            //:: all file chunks acked
            done = 1;
            finnum = acknum;
            set_FIN_pkt(finnum); //-> packet_buf
            tcpfsm.event = SENDDATA;
        }

        switch (tcpfsm.event) {

            case SENDDATA:
                tcpfsm.event = WAIT;
                if (done == 1){
                    send_FIN_pkt();
#ifdef DEBUG
                    printf("FIN send\n");
#endif
                    break;
                }
                while (!eof){
                    //: add data to window if there is space and send it
                    ind = add_to_window(buffer, bufferlen, npkt, seq);
                    if ( ind != -1 ){
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
                //printf("SENDDATA upto %d\n", npkt);
                break;

            case ACK:
                //:: handle a cumulative ACK

                tcpfsm.event = WAIT;
#ifdef DEBUG
                if (pkt_ind >= n_ackpkt) {
                    printf("n_ackpkt = %d\n", n_ackpkt);
                    error("shouldn't happen when tcpfsm.event == ACK");
                }
#endif
                // parse received packet
                unpack_tcphdr(ack_buf + pkt_ind, tmptcphdr);
                pkt_ind += MYTCPHDR_LEN; // len of ack pkt
                write_log(tmptcphdr);
                if (tmptcphdr.th_flags & TH_ACK){
                    acknum = tmptcphdr.th_ack;
                } else break;

                // all file data acked, try to ack FIN
                if (done == 1){
                    if (acknum == finnum + 1) done = 2;
                    continue;
                }
                if (ack_window(acknum) ){
                    // if true: successfully ack all packet (npkt < acknum),
                    // and make the spot in window available again
#ifdef DEBUG
                    printf("ack %d\n", acknum);
#endif
                    //: always try to send new data after each ACK
                    if (!eof){
                        tcpfsm.event = SENDDATA;
                    }
                    continue;// can skip WAIT state
                }
                break;

            case TIMEOUT:
                //:: retransmit unacked packet with least sequence number
                tcpfsm.event = WAIT;
                // resend FIN
                if (done == 1){
                    send_FIN_pkt();
                    break;
                }
                send_packet(tcpfsm.sendbase % cwnd);
#ifdef DEBUG
                printf("retransmit pkt %d\n", tcpfsm.sendbase);
#endif
                break;
            default:
                break;
        }

        if (pkt_ind < n_ackpkt) {
            // there is still ACK packets in buffer
            tcpfsm.event = ACK;// still ACK event
            continue;
        }

        //:: wait for new event (ack/timeout)
        // make sure sockfd in readfds before select()
        int ret_select = select(sockfd + 1, &readfds, NULL, NULL, &timeout);
        if (ret_select == -1)
            error("ERROR select()");
        else if (ret_select == 0) {
            FD_SET(sockfd, &readfds);
            tcpfsm.event = TIMEOUT;
            continue;
        }

        //:: receive ACK packets
        if (FD_ISSET(sockfd, &readfds) ){ //~~ after receiver sock closed, here always being true
            n_ackpkt = recv(sockfd, ack_buf, sizeof ack_buf, 0);
            if (n_ackpkt == 0){
                tcpfsm.event = WAIT;
                continue;
            }
            pkt_ind = 0;
            // since multiple pkts can huddle, receiver should send only fixed-length pkts
            // otherwise need to provide `option` so as to differentiate tcp pkts
            tcpfsm.event = ACK;
        } else
            error("`if` should be true or never be reached after select()");

    }

    //:: Statistics
    if (tcpfsm.transmax){
        printf("Max transmission times reached. Fail to send the file\n");
    } else {
        printf("Delivery completed successfully\n");
    }
    printf("Total bytes sent = %d\n", nbytesent);
    printf("Segments sent = %d\n", nseg);
    printf("Segments retransmitted = %.2f %% ", 100.0 * (nseg - npkt) / nseg );//~~ rename npkt->ndata

    fclose(fp);
    fclose(fp_log);

    close(sockfd);
    close(sockfd_tcp);
    close(sockfd_udp);

    return 0;
}
