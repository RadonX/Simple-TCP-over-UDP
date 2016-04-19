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

static tcpdata *window;
static int cwnd;

enum EVENT {READDATA, ACK, TIMEOUT, WAIT};
static enum EVENT event;

struct TCPFSM {
    int wndbase;
    int state;
};
static struct TCPFSM tcpfsm;


inline void init_window()
{
    window = (tcpdata *) calloc(cwnd, sizeof (tcpdata));
}

void init_tcpfsm()
{
    init_window();
    tcpfsm.wndbase = 0;
    tcpfsm.state = 0;
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

void ack_window(int index)
{
    int ind = index % cwnd; // (index = npkt) == seq
    window[ind].state = 0;
}
