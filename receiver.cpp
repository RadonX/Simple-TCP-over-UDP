#include "mytcp.h"
#include "sock.h"
#include "tcpstate.h"

#include "sharelib.h"

//#define DEBUG

struct mytcphdr tcp_hdr;
unsigned char packet_buf[MYTCPHDR_LEN];
int sockfd_tcp;
FILE* fp_log;

void write_log(struct mytcphdr &tcp_hdr )
{
    log_timestamp(logbuffer);
    log_tcphdr(logbuffer+TIMESTAMP_LEN, tcp_hdr);
    fprintf(fp_log, logbuffer);
//    puts(logbuffer);
    fprintf(fp_log, "\n");
}

void send_ack(int ack){
    set_tcphdr_ack(tcp_hdr, ack);
    pack_tcphdr(packet_buf, tcp_hdr);
    send(sockfd_tcp, packet_buf, sizeof(packet_buf), 0);
}

int main(int argc, char *argv[]){
    
    //:: parse arguments
    // > receiver file.pdf 20000 127.0.0.1 20001 logfile.txt
    if (argc != 6) {
        printf("Usage: %s <filename> <listening_port> <sender_IP> <sender_port> <log_filename>\n", argv[0]);
        exit(1);
    }
    int srcport = atoi(argv[2]), dstport = atoi(argv[4]);
    char *filename = argv[1];
    char *logfile = argv[5];
    //:: set remote & local sockaddr
    struct sockaddr_storage send_addr, recv_addr;
    socklen_t send_addrlen, recv_addrlen;
    int aifamily = set_sockaddr(send_addr, send_addrlen, argv[3], argv[4]);
    set_sockaddr(recv_addr, recv_addrlen, NULL, argv[2], aifamily);


    unsigned char buffer[MYTCPHDR_LEN + TCP_DATA_SIZE];
    int n;

    //: create a TCP socket for ack
    sockfd_tcp = create_and_bind_socket(recv_addr, recv_addrlen, aifamily, SOCK_STREAM);
    //: connect to sender
    if (connect(sockfd_tcp, (struct sockaddr *) &send_addr, send_addrlen ) < 0)
        error("ERROR connecting");
    n = read(sockfd_tcp, buffer, 255);
    if (n < 0) error("ERROR reading from socket_tcp");
    printf("Connected: %s\n", buffer);

    //: create a socket for UDP
    int sockfd_udp = create_and_bind_socket(recv_addr, recv_addrlen, aifamily, SOCK_DGRAM);

    //:: open files
    FILE* fp = fopen(filename, "wb");
    if (fp == NULL)
        error("Can't open the file.\n");
    fp_log = open_logfile(logfile);
    if (fp_log == NULL)
        error("Can't open the log file.\n");

    //: init tcp header for sending
    struct mytcphdr tmptcphdr;
    set_tcphdr(tcp_hdr, srcport, dstport);


    struct sockaddr_in from;
    socklen_t fromlen;

    tcp_seq expseq = 0;

    while (true){

        //:  receive data from UDP socket
        n = recvfrom(sockfd_udp, buffer, sizeof(buffer), 0, (struct sockaddr *) &from, &fromlen);
        if (n < MYTCPHDR_LEN) error("ERROR recvfrom");

        unpack_tcphdr(buffer, tmptcphdr);
        write_log(tmptcphdr);


        if (verify_checksum(buffer, n) && expseq == tmptcphdr.th_seq){
            fwrite(buffer + MYTCPHDR_LEN, sizeof(char), n - MYTCPHDR_LEN, fp);//~~ sizeof(buffer)??
            expseq++;
        } // else: drop corrupted/out-of-order pkt
        send_ack(expseq);
        write_log(tcp_hdr);

        if (tmptcphdr.th_flags & TH_FIN) break;

#ifdef DEBUG
        printf("  >> from %s port %d\n", inet_ntoa(from.sin_addr), ntohs(from.sin_port));
        if (n == MYTCPHDR_LEN){
            printf("if it's FIN packet, shouldn't be here");
            break;
        }
#endif


        //~~ todo: break when sockfd is closed
    }

    fclose(fp);
    fclose(fp_log);
    printf("Delivery completed successfully\n");

    close(sockfd_tcp);
    close(sockfd_udp);

    return 0;
}