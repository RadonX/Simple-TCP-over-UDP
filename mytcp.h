
// ref: http://read.cs.ucla.edu/click/doxygen/tcp_8h-source.html
// #include <netinet/tcp.h>

#include <stdlib.h>
#include <sys/types.h>
#include "pack.h"

#include <string.h>
#include "stdio.h"

//#define __DARWIN_BYTE_ORDER  __DARWIN_BIG_ENDIAN
typedef	__uint32_t tcp_seq;


struct mytcphdr {
    unsigned short	th_sport;	/* 0-1  source port */
    unsigned short	th_dport;	/* 2-3  destination port */
    tcp_seq	th_seq;			/* 4-7  sequence number */
    tcp_seq	th_ack;			/* 8-11 acknowledgement number */
    // __DARWIN_BYTE_ORDER == __DARWIN_BIG_ENDIAN /* 12 */
    unsigned int	th_off:4,	/* data offset */
            th_x2:4;	/* (unused) */
    unsigned char	th_flags;   /* 13   flags */
#define	TH_FIN	0x01
#define	TH_SYN	0x02
#define	TH_RST	0x04
#define	TH_PUSH	0x08
#define	TH_ACK	0x10
#define	TH_URG	0x20
//#define	TH_ECE	0x40
//#define	TH_CWR	0x80
//#define	TH_FLAGS	(TH_FIN|TH_SYN|TH_RST|TH_ACK|TH_URG|TH_ECE|TH_CWR)

    unsigned short	th_win;		/* 14-15    window */
    unsigned short	th_sum;		/* 16-17    checksum */
    unsigned short	th_urp;		/* 18-19    urgent pointer */
};

#define MYTCPHDR_LEN sizeof(struct mytcphdr)
#define CHECKSUM_IND 16


inline void init_tcphdr(struct mytcphdr &tcp_hdr) //~~ define
{
    bzero((char *) &tcp_hdr, MYTCPHDR_LEN);
}

void set_tcphdr(struct mytcphdr &tcp_hdr, unsigned short sport, unsigned short dport,
                unsigned short th_win = 65535, int th_off = MYTCPHDR_LEN / 4)
{
    init_tcphdr(tcp_hdr);

    tcp_hdr.th_sport = sport;
    tcp_hdr.th_dport = dport;
//    tcp_hdr.th_seq = seq;

    tcp_hdr.th_off = th_off;
//    tcp_hdr.th_x2 = 0;

    tcp_hdr.th_win = th_win;
//    tcp_hdr.th_sum = tcp4_checksum (iphdr, tcphdr);
//    tcp_hdr.th_urp = 0; // only valid if URG flag is set
}

inline void set_tcphdr_ack(struct mytcphdr &tcp_hdr, tcp_seq ack)
{
    tcp_hdr.th_flags |= TH_ACK;
    tcp_hdr.th_ack = ack;
}

inline void unpack_tcphdr(unsigned char *buffer, struct mytcphdr &tcp_hdr)
{
    int offset;
    unpack(buffer, "HHLLCCHHH", &tcp_hdr.th_sport, &tcp_hdr.th_dport,
           &tcp_hdr.th_seq, &tcp_hdr.th_ack, &offset,
           &tcp_hdr.th_flags, &tcp_hdr.th_win, &tcp_hdr.th_sum, &tcp_hdr.th_urp);
//    tcp_hdr.th_off = offset >> 4;
}

char flags[] = "UAPRSF" ;
int flags_val[] = {TH_URG, TH_ACK, TH_PUSH, TH_RST, TH_SYN, TH_FIN };

void log_tcphdr(char *logbuffer, struct mytcphdr &tcp_hdr)
{
    char flagsbuffer[7] = "null";
    int i, j;
    for (i = 0, j = 0; i < 7; i++){
        if (tcp_hdr.th_flags & flags_val[i]){
            flagsbuffer[j] = flags[i];
            j++;
        }
    }
    if (j != 0)
        flagsbuffer[j] = '\0';
    sprintf(logbuffer, "%d, %d, %d, %d, %s",
            tcp_hdr.th_sport, tcp_hdr.th_dport, tcp_hdr.th_seq, tcp_hdr.th_ack, flagsbuffer);
}


inline void pack_tcphdr(unsigned char *buffer, const struct mytcphdr &tcp_hdr)
{
    pack(buffer, "HHLLCCHHH", tcp_hdr.th_sport, tcp_hdr.th_dport,
         tcp_hdr.th_seq, tcp_hdr.th_ack, (tcp_hdr.th_off << 4) + tcp_hdr.th_x2,
         tcp_hdr.th_flags, tcp_hdr.th_win, tcp_hdr.th_sum, tcp_hdr.th_urp);

}

/*
 * The checksum is initialized to zero. Place the return value in
   the checksum field of a packet.  When the packet is received,
   check the checksum, by passing in the checksum field of the
   packet and the data.
   If the result is zero, then the checksum has not detected an error.
 */


u_short calc_checksum(u_char *packet_buf, int packet_len) //16-bit
{

    register u_int sum = 0;//~~
    register u_short *ptr = (u_short *) packet_buf;

    while(packet_len >= 2)
    {
        sum = sum + *(ptr)++;
        packet_len -= 2;
    }
    if (packet_len > 0)
        sum = sum + *((u_char *) ptr);

    // fold 32-bit sum to 16 bits
    while (sum>>16)
        sum = (sum & 0xFFFF) + (sum >> 16);
    return (~sum);
}

bool verify_checksum(u_char *packet_buf, int packet_len)
{
    // u_short sum = *((u_short *) (packet_buf + CHECKSUM_IND)) ;
    return 0 == calc_checksum(packet_buf, packet_len);
}