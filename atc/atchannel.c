/* //device/system/reference-ril/atchannel.c
**
** Copyright 2006, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/
#include "../util/log.h"
#include "atchannel.h"
#include "at_tok.h"
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <sys/timeb.h>
#include <stdarg.h>
#include <sys/epoll.h>
#include <poll.h>
#include <termios.h>
#include <sys/ioctl.h>
// #include "QMIThread.h"
// #define LOGE dbg_time
// #define LOGD dbg_time
// FILE *logfilefp = NULL;

#define NUM_ELEMS(x) (sizeof(x)/sizeof(x[0]))

#define MAX_AT_RESPONSE sizeof(cm_recv_buf)
#define HANDSHAKE_RETRY_COUNT 4
#define HANDSHAKE_TIMEOUT_MSEC 1000

static pthread_t s_tid_reader;
static int s_fd = -1;    /* fd of the AT channel */
static ATUnsolHandler s_unsolHandler;
static int s_atc_proxy = 0;    /* fd of the AT channel */

/* for input buffering */
unsigned int cm_recv_buf[1024];
pthread_mutex_t cm_command_mutex;
pthread_cond_t cm_command_cond;


static char *s_ATBuffer = (char *)cm_recv_buf;
static char *s_ATBufferCur = (char *)cm_recv_buf;

static int s_readCount = 0;

#if AT_DEBUG
void  AT_DUMP(const char*  prefix, const char*  buff, int  len)
{
    if (len < 0)
        len = strlen(buff);
    LOGD("%.*s", len, buff);
}
#endif

// const char * get_time(void) {
//     static char time_buf[128];
//     struct timeval  tv;
//     time_t time;
//     suseconds_t millitm;
//     struct tm *ti;

//     gettimeofday (&tv, NULL);

//     time= tv.tv_sec;
//     millitm = (tv.tv_usec + 500) / 1000;

//     if (millitm == 1000) {
//         ++time;
//         millitm = 0;
//     }

//     ti = localtime(&time);
//     sprintf(time_buf, "%4d-%02d-%02d %02d:%02d:%02d:%03d", ti->tm_year + 1900, ti->tm_mon+1, ti->tm_mday, ti->tm_hour, ti->tm_min, ti->tm_sec, (int)millitm);
//     return time_buf;
// }

unsigned long clock_msec(void)
{
	struct timespec tm;
	clock_gettime( CLOCK_MONOTONIC, &tm);
	return (unsigned long)(tm.tv_sec*1000 + (tm.tv_nsec/1000000));
}



static void setTimespecRelative(struct timespec *p_ts, long long msec)
{
    struct timeval tv;

    gettimeofday(&tv, (struct timezone *) NULL);

    /* what's really funny about this is that I know
       pthread_cond_timedwait just turns around and makes this
       a relative time again */
    p_ts->tv_sec = tv.tv_sec + (msec / 1000);
    p_ts->tv_nsec = (tv.tv_usec + (msec % 1000) * 1000L ) * 1000L;
    if ((unsigned long)p_ts->tv_nsec >= 1000000000UL) {
        p_ts->tv_sec += 1;
        p_ts->tv_nsec -= 1000000000UL;
    }
}

int pthread_cond_timeout_np(pthread_cond_t *cond, pthread_mutex_t * mutex, unsigned msecs) {
    if (msecs != 0) {
        unsigned i;
        unsigned t = msecs/4;
        int ret = 0;

        if (t == 0)
            t = 1;

        for (i = 0; i < msecs; i += t) {
            struct timespec ts;
            setTimespecRelative(&ts, t);
//very old uclibc do not support pthread_condattr_setclock(CLOCK_MONOTONIC)
            ret = pthread_cond_timedwait(cond, mutex, &ts); //to advoid system time change
            if (ret != ETIMEDOUT) {
                if(ret) LOGD("ret=%d, msecs=%u, t=%u", ret, msecs, t);
                break;
            }
        }

        return ret;
    } else {
        return pthread_cond_wait(cond, mutex);
    }
}




/*
 * for p_cur pending command
 * these are protected by s_commandmutex
 */
static ATCommandType s_type;
static const char *s_responsePrefix = NULL;
static const char *s_smsPDU = NULL;
static const char *s_raw_data = NULL;
static size_t s_raw_len;
static ATResponse *sp_response = NULL;

static void (*s_onTimeout)(void) = NULL;
static void (*s_onReaderClosed)(void) = NULL;
static int s_readerClosed;

static void onReaderClosed();
static int writeCtrlZ (const char *s);
static int writeline (const char *s);
static int writeraw (const char *s, size_t len);

static void sleepMsec(long long msec)
{
    struct timespec ts;
    int err;

    ts.tv_sec = (msec / 1000);
    ts.tv_nsec = (msec % 1000) * 1000 * 1000;

    do {
        err = nanosleep (&ts, &ts);
    } while (err < 0 && errno == EINTR);
}

/** returns 1 if line starts with prefix, 0 if it does not */
int strStartsWith(const char *line, const char *prefix)
{
    for ( ; *line != '\0' && *prefix != '\0' ; line++, prefix++) {
        if (*line != *prefix) {
            return 0;
        }
    }

    return *prefix == '\0';
}

/** add an intermediate response to sp_response*/
static void addIntermediate(const char *line)
{
    ATLine *p_new;

    p_new = (ATLine  *) malloc(sizeof(ATLine));

    p_new->line = strdup(line);

    /* note: this adds to the head of the list, so the list
       will be in reverse order of lines received. the order is flipped
       again before passing on to the command issuer */
    p_new->p_next = sp_response->p_intermediates;
    sp_response->p_intermediates = p_new;
}


/**
 * returns 1 if line is a final response indicating error
 * See 27.007 annex B
 * WARNING: NO CARRIER and others are sometimes unsolicited
 */
static const char * s_finalResponsesError[] = {
    "ERROR",
    "+CMS ERROR:",
    "+CME ERROR:",
    "NO CARRIER", /* sometimes! */
    "NO ANSWER",
    "NO DIALTONE",
    "COMMAND NOT SUPPORT",
};
static int isFinalResponseError(const char *line)
{
    size_t i;

    for (i = 0 ; i < NUM_ELEMS(s_finalResponsesError) ; i++) {
        if (strStartsWith(line, s_finalResponsesError[i])) {
            return 1;
        }
    }

    return 0;
}

/**
 * returns 1 if line is a final response indicating success
 * See 27.007 annex B
 * WARNING: NO CARRIER and others are sometimes unsolicited
 */
static const char * s_finalResponsesSuccess[] = {
    "OK",
    "+QIND: \"FOTA\",\"END\",0",
    "CONNECT"       /* some stacks start up data on another channel */
};

static int isFinalResponseSuccess(const char *line)
{
    size_t i;

    for (i = 0 ; i < NUM_ELEMS(s_finalResponsesSuccess) ; i++) {
        if (strStartsWith(line, s_finalResponsesSuccess[i])) {
            return 1;
        }
    }

    return 0;
}

#if 0
/**
 * returns 1 if line is a final response, either  error or success
 * See 27.007 annex B
 * WARNING: NO CARRIER and others are sometimes unsolicited
 */
static int isFinalResponse(const char *line)
{
    return isFinalResponseSuccess(line) || isFinalResponseError(line);
}
#endif

/**
 * returns 1 if line is the first line in (what will be) a two-line
 * SMS unsolicited response
 */
static const char * s_smsUnsoliciteds[] = {
   "+CMT:",
    "+CDS:",
    "+CBM:",
    "+CMTI:"
};
static int isSMSUnsolicited(const char *line)
{
    size_t i;

    for (i = 0 ; i < NUM_ELEMS(s_smsUnsoliciteds) ; i++) {
        if (strStartsWith(line, s_smsUnsoliciteds[i])) {
            return 1;
        }
    }

    return 0;
}


/** assumes s_commandmutex is held */
static void handleFinalResponse(const char *line)
{
    sp_response->finalResponse = strdup(line);

    pthread_cond_signal(&cm_command_cond);
}

static void handleUnsolicited(const char *line)
{
    if (s_unsolHandler != NULL) {
        s_unsolHandler(line, NULL);
    }
}

static int is_asciicode(const char *line) {
    if((line == NULL) || strstr(line, "OK") || strstr(line, "AT") || strstr(line, ":")) return 0;
    while (*line) {
        if(*line == '_' || *line == '-' || *line == ' ' || *line == '.' || *line == '+') {
            line++;
            continue;
        }
        if (!isalnum(*line)) {
            return 0;
        }
        line++;
    }
    return 1;
}

static void processLine(const char *line)
{
    pthread_mutex_lock(&cm_command_mutex);

    if (sp_response == NULL) {
        /* no command pending */
        handleUnsolicited(line);
    } else if (s_raw_data != NULL && 0 == strcmp(line, "CONNECT")) {
        usleep(500*1000); //for EC20
        writeraw(s_raw_data, s_raw_len);
        s_raw_data = NULL;
    } else if (isFinalResponseSuccess(line)) {
        if(s_atc_proxy)
            handleUnsolicited(line);
        sp_response->success = 1;
        handleFinalResponse(line);
    } else if (isFinalResponseError(line)) {
        if(s_atc_proxy) 
            handleUnsolicited(line);
        sp_response->success = 0;
        handleFinalResponse(line);
    } else if (s_smsPDU != NULL && 0 == strcmp(line, "> ")) {
        // See eg. TS 27.005 4.3
        // Commands like AT+CMGS have a "> " prompt
        writeCtrlZ(s_smsPDU);
        s_smsPDU = NULL;
    } else switch (s_type) {
        case NO_RESULT:
            handleUnsolicited(line);
            break;
        case NUMERIC:
            if (sp_response->p_intermediates == NULL
                && isdigit(line[0])
            ) {
                addIntermediate(line);
            } else {
                /* either we already have an intermediate response or
                   the line doesn't begin with a digit */
                handleUnsolicited(line);
            }
            break;
        case SINGLELINE:
            if (sp_response->p_intermediates == NULL
                && strStartsWith (line, s_responsePrefix)
            ) {
                addIntermediate(line);
            } else {
                /* we already have an intermediate response */
                handleUnsolicited(line);
            }
            break;
        case MULTILINE:
            if (strStartsWith (line, s_responsePrefix)) {
                addIntermediate(line);
            } else {
                handleUnsolicited(line);
            }
        break;
        case ASCIICODE:
            if (sp_response->p_intermediates == NULL
                && is_asciicode(line)
            ) {
                addIntermediate(line);
            } else {
                /* either we already have an intermediate response or
                   the line doesn't begin with a digit */
                handleUnsolicited(line);
            }
            break;
        default: /* this should never be reached */
            // LOGE("Unsupported AT command type %d\n", s_type);
            handleUnsolicited(line);
        break;
    }

    pthread_mutex_unlock(&cm_command_mutex);
}


/**
 * Returns a pointer to the end of the next line
 * special-cases the "> " SMS prompt
 *
 * returns NULL if there is no complete line
 */
static char * findNextEOL(char *cur)
{
    if (cur[0] == '>' && cur[1] == ' ' && cur[2] == '\0') {
        /* SMS prompt character...not \r terminated */
        return cur+2;
    }

    // Find next newline
    while (*cur != '\0' && *cur != '\r' && *cur != '\n') cur++;

    return *cur == '\0' ? NULL : cur;
}


/**
 * Reads a line from the AT channel, returns NULL on timeout.
 * Assumes it has exclusive read access to the FD
 *
 * This line is valid only until the next call to readline
 *
 * This function exists because as of writing, android libc does not
 * have buffered stdio.
 */

static const char *readline()
{
    ssize_t count;

    char *p_read = NULL;
    char *p_eol = NULL;
    char *ret;

    /* this is a little odd. I use *s_ATBufferCur == 0 to
     * mean "buffer consumed completely". If it points to a character, than
     * the buffer continues until a \0
     */
    if (*s_ATBufferCur == '\0') {
        /* empty buffer */
        s_ATBufferCur = s_ATBuffer;
        *s_ATBufferCur = '\0';
        p_read = s_ATBuffer;
    } else {   /* *s_ATBufferCur != '\0' */
        /* there's data in the buffer from the last read */

        // skip over leading newlines
        while (*s_ATBufferCur == '\r' || *s_ATBufferCur == '\n')
            s_ATBufferCur++;

        p_eol = findNextEOL(s_ATBufferCur);

        if (p_eol == NULL) {
            /* a partial line. move it up and prepare to read more */
            size_t len;

            len = strlen(s_ATBufferCur);

            memmove(s_ATBuffer, s_ATBufferCur, len + 1);
            p_read = s_ATBuffer + len;
            s_ATBufferCur = s_ATBuffer;
        }
        /* Otherwise, (p_eol !- NULL) there is a complete line  */
        /* that will be returned the while () loop below        */
    }

    while (p_eol == NULL) {
        if (0 == MAX_AT_RESPONSE - (p_read - s_ATBuffer)) {
            LOGE("ERROR: Input line exceeded buffer\n");
            /* ditch buffer and start over again */
            s_ATBufferCur = s_ATBuffer;
            *s_ATBufferCur = '\0';
            p_read = s_ATBuffer;
        }

        do {
            while (s_fd > 0) {
                struct pollfd pollfds[1] = {{s_fd, POLLIN, 0}};
                int ret;
            
                do {
                    ret = poll(pollfds, 1, -1);
                } while ((ret < 0) && (errno == EINTR));

                if (pollfds[0].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                    break;
                } else if   (pollfds[0].revents & (POLLIN))  {
                    break;
                }
            };

            count = (s_fd == -1) ? 0 : read(s_fd, p_read,
                            MAX_AT_RESPONSE - (p_read - s_ATBuffer));
        } while (count < 0 && errno == EINTR);

        if (count > 0) {
            AT_DUMP( "<< ", p_read, count );
            s_readCount += count;

            p_read[count] = '\0';

            // skip over leading newlines
            while (*s_ATBufferCur == '\r' || *s_ATBufferCur == '\n')
                s_ATBufferCur++;

            p_eol = findNextEOL(s_ATBufferCur);
            p_read += count;
        } else if (count <= 0) {
            /* read error encountered or EOF reached */
            at_close();
            if(count == 0) {
                LOGD("atchannel: EOF reached");
            } else {
                LOGD("atchannel: read error %s", strerror(errno));
            }
            return NULL;
        }
    }

    /* a full line in the buffer. Place a \0 over the \r and return */

    ret = s_ATBufferCur;
    *p_eol = '\0';
    s_ATBufferCur = p_eol + 1; /* this will always be <= p_read,    */
                              /* and there will be a \0 at *p_read */
#ifdef AT_DEBUG_FULL
    LOGD("AT< %s", ret);
#else
    LOGD("%s", ret);
#endif
    return ret;
}


static void onReaderClosed()
{
    LOGE("%s", __func__);
    if (s_onReaderClosed != NULL && s_readerClosed == 0) {

        pthread_mutex_lock(&cm_command_mutex);

        s_readerClosed = 1;

        pthread_cond_signal(&cm_command_cond);

        pthread_mutex_unlock(&cm_command_mutex);

        s_onReaderClosed();
        if (s_fd >= 0) {
            close(s_fd);
        }
        s_fd = -1;
        exit(0);
    }else{
        if (s_fd >= 0) {
            close(s_fd);
        }
        s_fd = -1;
        exit(0);
    }
    
}


static void *readerLoop(void *arg)
{
    (void)arg;

    for (;;) {
        const char * line;

        line = readline();

        if (line == NULL) {
            break;
        }

        if(isSMSUnsolicited(line)) {
            char *line1;
            const char *line2;

            // The scope of string returned by 'readline()' is valid only
            // till next call to 'readline()' hence making a copy of line
            // before calling readline again.
            line1 = strdup(line);
            line2 = readline();

            if (line2 == NULL) {
                break;
            }

            if (s_unsolHandler != NULL) {
                s_unsolHandler (line1, line2);
            }
            free(line1);
        } else {
            processLine(line);
        }
    }

    onReaderClosed();

    return NULL;
}

/**
 * Sends string s to the radio with a \r appended.
 * Returns AT_ERROR_* on error, 0 on success
 *
 * This function exists because as of writing, android libc does not
 * have buffered stdio.
 */
static int writeline (const char *s)
{
    size_t cur = 0;
    size_t len = strlen(s);
    ssize_t written;
    static char at_command[64];

    if (s_fd < 0 || s_readerClosed > 0) {
        return AT_ERROR_CHANNEL_CLOSED;
    }
#ifdef AT_DEBUG_FULL
    LOGD("AT> %s", s);
#endif

    AT_DUMP( ">> ", s, strlen(s) );

#if 1 //send '\r' maybe fail via USB controller: Intel Corporation 7 Series/C210 Series Chipset Family USB xHCI Host Controller (rev 04)
    if (len < (sizeof(at_command) - 1)) {
        strcpy(at_command, s);
        at_command[len++] = '\r';
        s = (const char *)at_command;
    }
#endif

    /* the main string */
    while (cur < len) {
        do {
            written = write (s_fd, s + cur, len - cur);
        } while (written < 0 && errno == EINTR);

        if (written < 0) {
            return AT_ERROR_GENERIC;
        }

        cur += written;
    }

#if 1 //Quectel send '\r' maybe fail via USB controller: Intel Corporation 7 Series/C210 Series Chipset Family USB xHCI Host Controller (rev 04)
    if (s == (const char *)at_command) {
        return 0;
    }
#endif

    /* the \r  */

    do {
        written = write (s_fd, "\r" , 1);
    } while ((written < 0 && errno == EINTR) || (written == 0));
    
    if (written < 0) {
        return AT_ERROR_GENERIC;
    }

    return 0;
}
static int writeCtrlZ (const char *s)
{
    size_t cur = 0;
    size_t len = strlen(s);
    ssize_t written;

    if (s_fd < 0 || s_readerClosed > 0) {
        return AT_ERROR_CHANNEL_CLOSED;
    }

    LOGD("AT> %s^Z", s);

    AT_DUMP( ">* ", s, strlen(s) );

    /* the main string */
    while (cur < len) {
        do {
            written = write (s_fd, s + cur, len - cur);
        } while (written < 0 && errno == EINTR);

        if (written < 0) {
            return AT_ERROR_GENERIC;
        }

        cur += written;
    }

    /* the ^Z  */

    do {
        written = write (s_fd, "\032" , 1);
    } while ((written < 0 && errno == EINTR) || (written == 0));

    if (written < 0) {
        return AT_ERROR_GENERIC;
    }

    return 0;
}

static int writeraw (const char *s, size_t len) {
    size_t cur = 0;
    ssize_t written;

    if (s_fd < 0 || s_readerClosed > 0) {
        return AT_ERROR_CHANNEL_CLOSED;
    }

    /* the main string */
    while (cur < len) {
        struct pollfd pollfds[1] = {{s_fd, POLLOUT, 0}};
        int ret;

        ret = poll(pollfds, 1, -1);
        if (ret <= 0)
            break;
            
        do {
            written = write (s_fd, s + cur, len - cur);
        } while (written < 0 && errno == EINTR);

        if (written < 0) {
            return AT_ERROR_GENERIC;
        }

        cur += written;
    }

    if (written < 0) {
        return AT_ERROR_GENERIC;
    }

    return cur;
}

static void clearPendingCommand()
{
    if (sp_response != NULL) {
        at_response_free(sp_response);
    }

    sp_response = NULL;
    s_responsePrefix = NULL;
    s_smsPDU = NULL;
}



static struct {
    uint32_t cbaud;
    uint32_t nspeed;
} speedTab[] = {
    { B300, 300    },
    { B1200, 1200   },
    { B2400, 2400   },
    { B4800, 4800   },
    { B9600, 9600   },
    { B19200, 19200  },
    { B38400, 38400  },
    { B57600, 57600  },
    { B115200, 115200 },
    { 0, 0 }
};

static struct termios origTTYAttrs;

int32_t uart_open(const char *device, uint32_t bps, uint32_t dataBits, uint32_t parity,
                              uint32_t stopBits, uint32_t rtsCts, uint32_t xOnXOff, int32_t timeout)
{
    uint32_t i;
    int32_t serial = -1;
    struct termios ttyAttrs = { 0 };

    /* Check if baud rate is supported. Return -1 if not. */
    for (i = 0; speedTab[i].nspeed != 0; i++) {
        if (bps == speedTab[i].nspeed) {
            break;
        }
    }
    if (speedTab[i].nspeed == 0) {
        LOGE("Baud rate not supported %s - %s(%d).",
                (char *)device,
                strerror(errno), errno);
        goto error;
    }

    /* Open the serial port read/write, with no controlling terminal, and don't wait for a
     * connection. The O_NONBLOCK flag also causes subsequent I/O on the device to be non-blocking. */
    serial = open((char *)device, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (serial == -1) {
        LOGE("Error opening serial port %s - %s(%d).",
                (char *)device,
                strerror(errno), errno);
        goto error;
    }

    /* Note that open() follows POSIX semantics: multiple open() calls to the same file will succeed
     * unless the TIOCEXCL ioctl is issued. This will prevent additional opens except by root-owned
     * processes. */
    if (ioctl(serial, TIOCEXCL) == -1) {
        LOGE("Error setting TIOCEXCL on %s - %s(%d).",
                (char *)device,
                strerror(errno), errno);
        goto error;
    }

    /* Get the current options and save them so we can restore the default settings later. */
    if (tcgetattr(serial, &origTTYAttrs) == -1) {
        LOGE("Error getting tty attributes %s - %s(%d).",
                (char *)device,
                strerror(errno), errno);
        goto error;
    }
    /* The serial port attributes such as timeouts and baud rate are set by modifying the termios
     * structure and then calling tcsetattr to cause the changes to take effect. Note that the changes
     * will not take effect without the tcsetattr() call. */
    ttyAttrs = origTTYAttrs;

    /* Now that the device is open, clear the O_NONBLOCK flag so subsequent I/O will block. */
    if (fcntl(serial, F_SETFL, 0) == -1) {
        LOGE("Error clearing O_NONBLOCK %s - %s(%d).",
                (char *)device,
                strerror(errno), errno);
        goto error;
    }

    /* Configure baud rate. */
    if (cfsetspeed(&ttyAttrs, speedTab[i].cbaud) == -1) {
        LOGE("Error setting baud rate %s - %s(%d).",
                (char *)device,
                strerror(errno), errno);
        goto error;
    }

    /* Set raw input (non-canonical) mode. */
    cfmakeraw(&ttyAttrs);

    /* Input modes. */
    /* If this bit is set, break conditions are ignored. */
    //   ttyAttrs.c_iflag = IGNBRK;

    /* Configure software flow control. */
    if (xOnXOff) {
        ttyAttrs.c_iflag |= IXON | IXOFF;
    } else {
        ttyAttrs.c_iflag &= ~(IXON | IXOFF);
    }

    /* Output modes. */
    ttyAttrs.c_oflag = 0;

    /* Control modes. */
    /* Ignore modem control lines. */
    ttyAttrs.c_cflag |= CLOCAL;
    /* Enable receiver. */
    ttyAttrs.c_cflag |= CREAD;

    /* Configure word length. Any number other than 5, 6 or 7 defaults to 8. */
    if (dataBits == 5) {
        ttyAttrs.c_cflag |= CS5;
    } else if (dataBits == 6) {
        ttyAttrs.c_cflag |= CS6;
    } else if (dataBits == 7) {
        ttyAttrs.c_cflag |= CS7;
    } else {
        ttyAttrs.c_cflag |= CS8;
    }

    /* Configure parity. Any number other than 1 or 2 defaults to 0. */
    if (parity == 1) {
        ttyAttrs.c_cflag |= PARENB | PARODD;
    } else if (parity == 2) {
        ttyAttrs.c_cflag |= PARENB;
    } else {
        /* No parity. */
        ttyAttrs.c_cflag &= ~PARENB;
    }

    /* Configure number of stop bits. Any number other than 2 defaults to 1. */
    if (stopBits == 2) {
        ttyAttrs.c_cflag |= CSTOPB;
    } else {
        /* 1 stop bit. */
        ttyAttrs.c_cflag &= ~CSTOPB;
    }

    /* Configure hardware flow control. */
    if (rtsCts) {
        ttyAttrs.c_cflag |= CRTSCTS;
    } else {
        ttyAttrs.c_cflag &= ~CRTSCTS;
    }

    /* Local modes. */
    ttyAttrs.c_lflag = 0;

    /* Special characters. */
    /* VMIN=0, VTIME=0: if data is available, read() returns immediately, with the lesser of the
     *                  number of bytes available, or the number of bytes requested. If no data is
     *                  available, read() returns 0.
     * VMIN=0, VTIME>0: VTIME specifies the limit for a timer in tenth of a second. The timer is
     *                  started when read() is called. read() returns either when at least one byte
     *                  of data is available, or when the timer expires. If the timer expires
     *                  without any input becoming available, read() returns 0.
     * VMIN>0, VTIME=0: read() blocks until the lesser of MIN bytes or the number of bytes requested
     *                  are available, and returns the lesser of these two values.
     * VMIN>0, VTIME>0: VTIME specifies the limit for a timer in tenth of a second. Once an initial
     *                  byte of input becomes available, the timer is restarted after each further
     *                  byte is received. read(2) returns either when the lesser of the number of
     *                  bytes requested or MIN byte have been read, or when the inter-byte timeout
     *                  expires. Because the timer is only started after the initial byte becomes
     *                  available, at least one byte will be read. */
    if (timeout < 0) {
        /* Block until character is received. No timeout configured. */
        ttyAttrs.c_cc[VMIN] = 1;
        ttyAttrs.c_cc[VTIME] = 0;
    } else {
        /* Block until character is received or timer expires. */
        ttyAttrs.c_cc[VMIN] = 0;
        ttyAttrs.c_cc[VTIME] = (cc_t)(timeout / 100);
    }

    /* Cause the new options to take effect immediately. */
    if (tcsetattr(serial, TCSANOW, &ttyAttrs) == -1) {
        LOGE("Error setting tty attributes %s - %s(%d).",
                (char *)device,
                strerror(errno), errno);
        goto error;
    }

    /* Success */
    return serial;

    /* Failure */
error:
    if (serial != -1) {
        close(serial);
    }

    return -1;
}

#define UART_OPEN(dev) uart_open(dev, 115200, 8, 0, 1, 0, 0, 100)
/**
 * Starts AT handler on stream "fd'
 * returns 0 on success, -1 on error
 */
int at_open(const char *dev, ATUnsolHandler h, int proxy)
{
    int ret;
    pthread_attr_t attr;

    s_fd = -1;
    s_fd = UART_OPEN(dev);
    s_unsolHandler = h;
    s_readerClosed = 0;
	s_atc_proxy = proxy;

    if (s_fd < 0){
        LOGE("at_open: open %s failed", dev);
        return -1;
    }

    s_responsePrefix = NULL;
    s_smsPDU = NULL;
    sp_response = NULL;
    
    pthread_attr_init (&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    ret = pthread_create(&s_tid_reader, &attr, readerLoop, NULL);

    if (ret < 0) {
        close(s_fd);
        LOGE("readerLoop create fail!");
        perror ("pthread_create\n");
        return -1;
    }

    return 0;
}

/* FIXME is it ok to call this from the reader and the command thread? */
void at_close()
{
    // dbg_time("at_close");
    if (s_fd >= 0) {
        close(s_fd);
    }
    s_fd = -1;

    pthread_mutex_lock(&cm_command_mutex);

    s_readerClosed = 0;

    pthread_cond_signal(&cm_command_cond);

    pthread_mutex_unlock(&cm_command_mutex);

    /* the reader thread should eventually die */
}

static ATResponse * at_response_new()
{
    return (ATResponse *) calloc(1, sizeof(ATResponse));
}

void at_response_free(ATResponse *p_response)
{
    ATLine *p_line;

    if (p_response == NULL) return;

    p_line = p_response->p_intermediates;

    while (p_line != NULL) {
        ATLine *p_toFree;

        p_toFree = p_line;
        p_line = p_line->p_next;

        free(p_toFree->line);
        free(p_toFree);
    }

    free (p_response->finalResponse);
    free (p_response);
}

/**
 * The line reader places the intermediate responses in reverse order
 * here we flip them back
 */
static void reverseIntermediates(ATResponse *p_response)
{
    ATLine *pcur,*pnext;

    pcur = p_response->p_intermediates;
    p_response->p_intermediates = NULL;

    while (pcur != NULL) {
        pnext = pcur->p_next;
        pcur->p_next = p_response->p_intermediates;
        p_response->p_intermediates = pcur;
        pcur = pnext;
    }
}

/**
 * Internal send_command implementation
 * Doesn't lock or call the timeout callback
 *
 * timeoutMsec == 0 means infinite timeout
 */
static int at_send_command_full_nolock (const char *command, ATCommandType type,
                    const char *responsePrefix, const char *smspdu,
                    long long timeoutMsec, ATResponse **pp_outResponse)
{
    int err = 0;

    if (!timeoutMsec)
        timeoutMsec = 15000;

    if(sp_response != NULL) {
        err = AT_ERROR_COMMAND_PENDING;
        goto error;
    }

    if (command != NULL)
        err = writeline (command);

    if (err < 0) {
        printf("%s errno: %d (%s)\n", __func__, errno, strerror(errno));
        goto error;
    }

    s_type = type;
    s_responsePrefix = responsePrefix;
    s_smsPDU = smspdu;
    sp_response = at_response_new();

    while (sp_response->finalResponse == NULL && s_readerClosed == 0) {
        err = pthread_cond_timeout_np(&cm_command_cond, &cm_command_mutex, timeoutMsec);

        if (err == ETIMEDOUT) {
            err = AT_ERROR_TIMEOUT;
            goto error;
        }
    }

    if (pp_outResponse == NULL) {
        at_response_free(sp_response);
    } else {
        /* line reader stores intermediate responses in reverse order */
        reverseIntermediates(sp_response);
        *pp_outResponse = sp_response;
    }

    sp_response = NULL;

    if(s_readerClosed > 0) {
        err = AT_ERROR_CHANNEL_CLOSED;
        goto error;
    }

    err = 0;
error:
    clearPendingCommand();

    return err;
}

/**
 * Internal send_command implementation
 *
 * timeoutMsec == 0 means infinite timeout
 */
static int at_send_command_full (const char *command, ATCommandType type,
                    const char *responsePrefix, const char *smspdu,
                    long long timeoutMsec, ATResponse **pp_outResponse)
{
    int err;

    if (0 != pthread_equal(s_tid_reader, pthread_self())) {
        /* cannot be called from reader thread */
        return AT_ERROR_INVALID_THREAD;
    }

    pthread_mutex_lock(&cm_command_mutex);

    err = at_send_command_full_nolock(command, type,
                    responsePrefix, smspdu,
                    timeoutMsec, pp_outResponse);

    pthread_mutex_unlock(&cm_command_mutex);

    if (err == AT_ERROR_TIMEOUT && s_onTimeout != NULL) {
        s_onTimeout();
    }

    return err;
}


/**
 * Issue a single normal AT command with no intermediate response expected
 *
 * "command" should not include \r
 * pp_outResponse can be NULL
 *
 * if non-NULL, the resulting ATResponse * must be eventually freed with
 * at_response_free
 */
int at_send_command (const char *command, ATResponse **pp_outResponse)
{
    int err;

    err = at_send_command_full (command, NO_RESULT, NULL,
                                    NULL, 0, pp_outResponse);

    return err;
}


int at_send_command_singleline (const char *command,
                                const char *responsePrefix,
                                 ATResponse **pp_outResponse)
{
    int err;

    err = at_send_command_full (command, SINGLELINE, responsePrefix,
                                    NULL, 0, pp_outResponse);

    if (err == 0 && pp_outResponse != NULL
        && (*pp_outResponse)->success > 0
        && (*pp_outResponse)->p_intermediates == NULL
    ) {
        /* successful command must have an intermediate response */
        at_response_free(*pp_outResponse);
        *pp_outResponse = NULL;
        return AT_ERROR_INVALID_RESPONSE;
    }

    return err;
}


int at_send_command_numeric (const char *command,
                                 ATResponse **pp_outResponse)
{
    int err;

    err = at_send_command_full (command, NUMERIC, NULL,
                                    NULL, 0, pp_outResponse);
    if (err == 0 && pp_outResponse != NULL
        && (*pp_outResponse)->success > 0
        && (*pp_outResponse)->p_intermediates == NULL
    ) {
        /* successful command must have an intermediate response */
        at_response_free(*pp_outResponse);
        *pp_outResponse = NULL;
        return AT_ERROR_INVALID_RESPONSE;
    }

    return err;
}

int at_send_command_asciicode (const char *command,
                                 ATResponse **pp_outResponse)
{
    int err;

    err = at_send_command_full (command, ASCIICODE, NULL,
                                    NULL, 0, pp_outResponse);
    if (err == 0 && pp_outResponse != NULL
        && (*pp_outResponse)->success > 0
        && (*pp_outResponse)->p_intermediates == NULL
    ) {
        /* successful command must have an intermediate response */
        at_response_free(*pp_outResponse);
        *pp_outResponse = NULL;
        return AT_ERROR_INVALID_RESPONSE;
    }

    return err;
}


int at_send_command_sms (const char *command,
                                const char *pdu,
                                const char *responsePrefix,
                                 ATResponse **pp_outResponse)
{
    int err;

    err = at_send_command_full (command, SINGLELINE, responsePrefix,
                                    pdu, 0, pp_outResponse);

    if (err == 0 && pp_outResponse != NULL
        && (*pp_outResponse)->success > 0
        && (*pp_outResponse)->p_intermediates == NULL
    ) {
        /* successful command must have an intermediate response */
        at_response_free(*pp_outResponse);
        *pp_outResponse = NULL;
        return AT_ERROR_INVALID_RESPONSE;
    }

    return err;
}

int at_send_command_multiline (const char *command,
                                const char *responsePrefix,
                                 ATResponse **pp_outResponse)
{
    int err;

    err = at_send_command_full (command, MULTILINE, responsePrefix,
                                    NULL, 0, pp_outResponse);

    return err;
}

int at_send_command_raw (const char *command,
                                const char *raw_data, unsigned int raw_len,
                                const char *responsePrefix,
                                 ATResponse **pp_outResponse)
{
    int err;

    s_raw_data = raw_data;
    s_raw_len = raw_len;
    err = at_send_command_full (command, SINGLELINE, responsePrefix,
                                    NULL, 0, pp_outResponse);

    return err;
}

/**
 * Periodically issue an AT command and wait for a response.
 * Used to ensure channel has start up and is active
 */

int at_handshake()
{
    int i;
    int err = 0;

    if (0 != pthread_equal(s_tid_reader, pthread_self())) {
        /* cannot be called from reader thread */
        return AT_ERROR_INVALID_THREAD;
    }

    pthread_mutex_lock(&cm_command_mutex);

    for (i = 0 ; i < HANDSHAKE_RETRY_COUNT ; i++) {
        /* some stacks start with verbose off */
        err = at_send_command_full_nolock ("ATE0Q0V1", NO_RESULT,
                    NULL, NULL, HANDSHAKE_TIMEOUT_MSEC, NULL);

        if (err == 0) {
            break;
        }
    }

    pthread_mutex_unlock(&cm_command_mutex);

    if (err == 0) {
        /* pause for a bit to let the input buffer drain any unmatched OK's
           (they will appear as extraneous unsolicited responses) */

        sleepMsec(HANDSHAKE_TIMEOUT_MSEC);
    }

    return err;
}

AT_CME_Error at_get_cme_error(const ATResponse *p_response)
{
    int ret;
    int err;
    char *p_cur;

    if (p_response == NULL)
        return CME_ERROR_NON_CME;

    if (p_response->success > 0) {
        return CME_SUCCESS;
    }

    if (p_response->finalResponse == NULL
        || !strStartsWith(p_response->finalResponse, "+CME ERROR:")
    ) {
        return CME_ERROR_NON_CME;
    }

    p_cur = p_response->finalResponse;
    err = at_tok_start(&p_cur);

    if (err < 0) {
        return CME_ERROR_NON_CME;
    }

    err = at_tok_nextint(&p_cur, &ret);

    if (err < 0) {
        return CME_ERROR_NON_CME;
    }

    return (AT_CME_Error) ret;
}

/** This callback is invoked on the command thread */
void at_set_on_timeout(void (*onTimeout)(void))
{
    s_onTimeout = onTimeout;
}

/**
 *  This callback is invoked on the reader thread (like ATUnsolHandler)
 *  when the input stream closes before you call at_close
 *  (not when you call at_close())
 *  You should still call at_close()
 */
void at_set_on_reader_closed(void (*onClose)(void))
{
    s_onReaderClosed = onClose;
}




