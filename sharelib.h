#include <stdio.h>
#include <string.h> //memcpy
#include <stdlib.h> //atoi()
#include <unistd.h> //close()
#include <arpa/inet.h> //inet_ntoa()
#include <sys/socket.h>
#include <netinet/in.h> //sockaddr_in
#include "time.h"

char logbuffer[50];
time_t logtimer;
struct tm* tm_info;
#define TIMESTAMP_LEN 10

inline void log_timestamp(char *logbuffer)
{
    time(&logtimer);
    tm_info = localtime(&logtimer);
    strftime(logbuffer, TIMESTAMP_LEN, "%H:%M:%S, ", tm_info);
}

FILE * open_logfile(char *logfile)
{
    FILE *fp_log;
    if ( strcmp(logfile, "stdout") == 0 ){
        fp_log = stdout;
    } else {
        fp_log = fopen(logfile, "wb");
    }
    return fp_log;
}
