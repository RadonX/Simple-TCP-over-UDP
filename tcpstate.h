#include <string.h>
#include <stdlib.h>
#include <sys/time.h>

#define MAX_CWND 128
#define TCP_DATA_SIZE 576 - 20 // no option field for TCP, 576
#define	RTO_MIN 1000
#define	RTO_MAX 5000
#define	RETRANS_MAX 12	/* max retransmision times */ //~~

#define max(a,b) ( (a)>(b)? (a): (b) )

struct segment{
    int state; // 0: available, 1: ready or sent
    int size;
    int seq;
    int n_trans;
    unsigned char data[TCP_DATA_SIZE];
    struct timeval sendtime;
};

struct segment *window;
int cwnd;

enum EVENT {SENDDATA, ACK, TIMEOUT, WAIT};

struct TCPFSM {
    int sendbase; // (index = seq) == npkt
    int n_dupack;
    enum EVENT event;
};
struct TCPFSM tcpfsm;


struct rtt_info {
    // in msec
    int rto;
    int srtt; /* smoothed RTT estimator */
    int rttvar; /* smoothed mean deviation */
    int G;
    int rtt; /* most recent measured RTT */
    float alpha;
    float beta;
    struct timeval tv;
//    int		rtt_nrexmt;	/* # times retransmitted: 0, 1, 2, ... */
//    uint32_t	rtt_base;	/* # sec since 1/1/1970 at start */
};
struct rtt_info rtt;

void init_rtt()
{
    rtt.alpha = 0.125;
    rtt.beta = 0.25;
    rtt.G = 50;
    rtt.rto = 1000;
    rtt.tv.tv_sec = rtt.rto/1000;
    rtt.tv.tv_usec = 0;
    rtt.srtt = 0;
    rtt.rttvar = 0;
}

void update_RTT(struct timeval &sendtime)
{
    struct timeval recvtime;
    gettimeofday(&recvtime, NULL);
    rtt.rtt = (recvtime.tv_sec * 1000 + recvtime.tv_usec / 1000) -
            (sendtime.tv_sec * 1000 + sendtime.tv_usec / 1000);
    if (rtt.srtt == 0){
        rtt.srtt = rtt.rtt;
        rtt.rttvar = rtt.srtt / 2;
    } else {
        rtt.rttvar = (1.0 - rtt.beta) * rtt.rttvar + rtt.beta * abs(rtt.rtt - rtt.srtt);
        rtt.srtt = (1.0 - rtt.alpha) * rtt.srtt + rtt.alpha * rtt.rtt;
    }
    rtt.rto = rtt.srtt + max(rtt.G, 4 * rtt.rttvar);
//    printf("%d -> ", rtt.rto);
    rtt.rto = rtt.rto > RTO_MAX ? RTO_MAX : (rtt.rto < RTO_MIN ? RTO_MIN : rtt.rto) ;
//    printf("%d\n", rtt.rto);
    rtt.tv.tv_sec = rtt.rto/1000;
    rtt.tv.tv_usec = (rtt.rto%1000) * 1000;
}


inline void init_window()
{
    window = (struct segment *) calloc(cwnd, sizeof (struct segment));
}

void init_tcpfsm()
{
    init_window();
    tcpfsm.sendbase = 0;
    tcpfsm.event = SENDDATA;
    tcpfsm.n_dupack = 0;
};


int add_to_window(unsigned char *buffer, int bufferlen, int index, int seq)
{
    int ind = index % cwnd;
    if (window[ind].state == 0){
        memcpy(window[ind].data, buffer, bufferlen);
        window[ind].size = bufferlen;
        window[ind].state = 1;
        window[ind].seq = seq;
        window[ind].n_trans = 0;
        return ind;
    }
    return -1;
}

inline void sent_from_window(int ind)
{
    gettimeofday(&(window[ind].sendtime), NULL);
    window[ind].n_trans++;
}

bool ack_window(int index)
{
    for (int i = index - 1; i >= tcpfsm.sendbase; i--){
        window[i % cwnd].state = 0;
    }
    int ind = (index-1) % cwnd;// (index = seq) == npkt
    if (index > tcpfsm.sendbase) {
        if (window[ind].n_trans == 1)
            update_RTT( window[ind].sendtime );
        tcpfsm.sendbase = index;
        tcpfsm.n_dupack = 1;
        return true; // window available
    } else if (index < tcpfsm.sendbase){
        if (window[ind].n_trans == 1)
            update_RTT( window[ind].sendtime );
        return false;
    } else{ // (index == tcpfsm.sendbase)
        tcpfsm.n_dupack++;
        return false;
    }
}
