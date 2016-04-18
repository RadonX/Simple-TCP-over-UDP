#include "string.h"

#define MAX_CWND 128
#define TCP_DATA_SIZE 256


struct tcpdata{
    int state; // 0: available, 1: ready
    int size;
    int seq;
    unsigned char data[TCP_DATA_SIZE];
};

static tcpdata *window;
static int cwnd;



struct TCPFSM {
    int wndbase;
    int dataready;
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
        tcpfsm.dataready++;
        return ind;
    }
    return -1;
}
