#include "string.h"
#include "stdlib.h"
#define MAX_CWND 128
#define TCP_DATA_SIZE 256


struct tcpdata{
    int state; // 0: available, 1: ready or sent
    int size;
    int seq;
    unsigned char data[TCP_DATA_SIZE];
};

tcpdata *window;
int cwnd;

enum EVENT {READDATA, ACK, TIMEOUT, WAIT};

struct TCPFSM {
    int sendbase; // (index = seq) == npkt
    int n_dupack;
    enum EVENT event;
};
struct TCPFSM tcpfsm;


inline void init_window()
{
    window = (tcpdata *) calloc(cwnd, sizeof (tcpdata));
}

void init_tcpfsm()
{
    init_window();
    tcpfsm.sendbase = 0;
    tcpfsm.event = READDATA;
    tcpfsm.n_dupack = 0;
//    tcpfsm.datasent = 0;
};


int add_data_to_window(unsigned char *buffer, int bufferlen, int index, int seq)
{
    int ind = index % cwnd;
    if (window[ind].state == 0){
        memcpy(window[ind].data, buffer, bufferlen);
        window[ind].size = bufferlen;
        window[ind].state = 1;
        window[ind].seq = seq;
        return ind;
    }
    return -1;
}

bool ack_window(int index)
{
    int ind;
    for (int i = index - 1; i >= tcpfsm.sendbase; i--){
        ind = index % cwnd; // (index = seq) == npkt
        window[ind].state = 0;
    }
    if (index > tcpfsm.sendbase) {
        tcpfsm.sendbase = index;
        tcpfsm.n_dupack = 1;
        return true;
    } else if (index == tcpfsm.sendbase){
        tcpfsm.n_dupack++;
    }
    return false;
}
