
// ref: http://read.cs.ucla.edu/click/doxygen/tcp_8h-source.html
// #include <netinet/tcp.h>

#include <stdlib.h>

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
#define	TH_ECE	0x40
#define	TH_CWR	0x80
//#define	TH_FLAGS	(TH_FIN|TH_SYN|TH_RST|TH_ACK|TH_URG|TH_ECE|TH_CWR)

    unsigned short	th_win;		/* 14-15    window */
    unsigned short	th_sum;		/* 16-17    checksum */
    unsigned short	th_urp;		/* 18-19    urgent pointer */
};

#define MYTCPHDR_LEN sizeof(struct mytcphdr)

#include <string.h>

void init_tcphdr(struct mytcphdr &tcp_hdr)
{
    bzero((char *) &tcp_hdr, MYTCPHDR_LEN);
}

void set_tcphdr(struct mytcphdr &tcp_hdr, int sport, int dport, int seq, int th_win = 65536, int ack = 0,
                int th_off = MYTCPHDR_LEN / 4, int th_flags = TH_ACK)
{
    //init
    bzero((char *) &tcp_hdr, MYTCPHDR_LEN);

    tcp_hdr.th_sport = htons(sport);
    tcp_hdr.th_dport = htons(dport);
    tcp_hdr.th_seq = htonl(seq);
    tcp_hdr.th_ack = htonl(ack);

    tcp_hdr.th_off = th_off;
//    tcp_hdr.th_x2 = 0;
    tcp_hdr.th_flags = th_flags;

    tcphdr.th_win = htons(th_win);
//    tcp_hdr.th_sum = tcp4_checksum (iphdr, tcphdr);
//    tcphdr.th_urp = htons(0); // only valid if URG flag is set

}