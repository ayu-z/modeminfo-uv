#include "log.h"

FILE *logfilefp = NULL;

const char *get_time(void) {
    static char time_buf[128];
    struct timeval tv;
    time_t time;
    suseconds_t millitm;
    struct tm *ti;

    gettimeofday(&tv, NULL);

    time = tv.tv_sec;
    millitm = (tv.tv_usec + 500) / 1000;

    if (millitm == 1000) {
        ++time;
        millitm = 0;
    }

    ti = localtime(&time);
    sprintf(time_buf, "%4d-%02d-%02d %02d:%02d:%02d:%03d",
            ti->tm_year + 1900, ti->tm_mon + 1, ti->tm_mday,
            ti->tm_hour, ti->tm_min, ti->tm_sec, (int)millitm);
    return time_buf;
}

void log_init(const char *log_file) { 
    logfilefp = fopen(log_file, "a"); 
    if (logfilefp == NULL) {
        perror("log_init");
        exit(1);
    }
}