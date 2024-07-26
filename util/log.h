#ifndef LOG_H
#define LOG_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#define RED_COLOR   "\033[31m"
#define GREEN_COLOR "\033[32m"
#define RESET_COLOR "\033[0m"

typedef enum {
    LOG_LV_ERROR,
    LOG_LV_INFO,
    LOG_LV_DEBUG,
    LOG_LV_ALL
} log_level_t;

#define dbg_time(level, fmt, ...) do { \
    if (level == LOG_LV_ERROR) \
        fprintf(stdout, RED_COLOR "[%s] " fmt RESET_COLOR "\n", get_time(), ##__VA_ARGS__); \
    else if (level == LOG_LV_DEBUG) \
        fprintf(stdout, GREEN_COLOR "[%s] " fmt RESET_COLOR "\n", get_time(), ##__VA_ARGS__); \
    fflush(stdout); \
    if (logfilefp) fprintf(logfilefp, "[%s] " fmt "\n", get_time(), ##__VA_ARGS__); \
} while (0)

#define LOGE(fmt, ...) dbg_time(LOG_LV_ERROR, fmt, ##__VA_ARGS__)
#define LOGD(fmt, ...) dbg_time(LOG_LV_DEBUG, fmt, ##__VA_ARGS__)

extern FILE *logfilefp;

const char *get_time(void);
void log_init(const char *log_file);

#endif // LOG_H
