#ifndef __HTTP_SERVER_H__
#define __HTTP_SERVER_H__

#include <uv.h>
#include <jansson.h>
#include <http_parser.h>


#define RESPONSE_OK "HTTP/1.1 200 OK\r\nContent-Length: %zu\r\nContent-Type: application/json\r\n\r\n%s"
#define RESPONSE_BAD_REQUEST "HTTP/1.1 400 Bad Request\r\nContent-Length: 16\r\nContent-Type: text/plain\r\n\r\nInvalid Request"
#define RESPONSE_NOT_FOUND "HTTP/1.1 404 Not Found\r\nContent-Length: 14\r\nContent-Type: text/plain\r\n\r\nNot Found"
#define URL_ERR_JSON "{\"err\": \"invalid url\"}"
#define MAX_BANDS 32
#define MAX_TOKEN_LENGTH 10

typedef struct {
    uv_work_t req;
    uv_stream_t* client;
    char* data;
    char* url_path;
    char* response_data;
    size_t response_length;
    int method;
} app_sched_t;

typedef struct {
    uv_tcp_t handle;
    http_parser parser;
    http_parser_settings settings;
    char* body;
    size_t body_length;
    char url_path[1024];
    int content_type_json;
    int header_type;
} http_requests_t;

#if 1
typedef struct {
    const char *url;
    void* (*request_handler)(void *);
    json_t* (*pack_handler)(void *);
} url_handler_t;
#else
typedef struct {
    const char *url;
    void* (*request_handler)(void);
    json_t* (*pack_handler)(void *);
} url_handler_t;
#endif

void on_new_connection(uv_stream_t* server, int status);
void proc_post_request(app_sched_t* work);
void proc_get_request(app_sched_t* work);
int prco_request_init(char *dev);

#endif // !__HTTP_SERVER_H__


