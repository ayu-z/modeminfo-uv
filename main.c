#include "http_server.h"
#include "util/log.h"
#include <bits/types/locale_t.h>
#include <stdio.h>
#include <signal.h>

#define  PORT_VAR_FILENAME "/var/run/cellmgt"
static void sig_action(int sig) {
    remove(PORT_VAR_FILENAME);
    exit(0);
}


int main(int argc, char *argv[])
{
    // log_init(NULL);
    struct sigaction sa;
    sa.sa_handler = sig_action;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL); 

    uv_loop_t* loop = uv_default_loop();
    uv_tcp_t server;
    struct sockaddr_in addr;

    uv_tcp_init(loop, &server);
    uv_ip4_addr("localhost", 0, &addr);
    uv_tcp_bind(&server, (const struct sockaddr*)&addr, 0);
    int r = uv_listen((uv_stream_t*) &server, 128, on_new_connection);
    if (r) {
        LOGE("Listen error %s", uv_strerror(r));
        return 1;
    }

    struct sockaddr_in sockname;
    int namelen = sizeof(sockname);
    uv_tcp_getsockname(&server, (struct sockaddr*)&sockname, &namelen);
    int port = ntohs(sockname.sin_port);
    LOGD("Listen port is %d", port);
    FILE *fp = fopen(PORT_VAR_FILENAME, "w");
    if (fp == NULL) {
        LOGE("Failed to open %s for writing", PORT_VAR_FILENAME);
        return 1;
    }
    fprintf(fp, "%d\n", port);
    fclose(fp);

    prco_request_init(argv[1]);

    return uv_run(loop, UV_RUN_DEFAULT);
}
