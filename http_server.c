#include "util/log.h"
#include "http_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
    buf->base = (char*) malloc(suggested_size);
    buf->len = suggested_size;
}

void on_write(uv_write_t* req, int status) {
    if (status) {
        LOGE("Write error %s", uv_strerror(status));
    }
    free(req);
}

void after_work_cb(uv_work_t* req, int status) {
    if (status == 0) {
        app_sched_t* work = (app_sched_t*) req->data;
        char response[2048];
        snprintf(response, sizeof(response), RESPONSE_OK, work->response_length, work->response_data);

        uv_write_t* write_req = (uv_write_t*) malloc(sizeof(uv_write_t));
        uv_buf_t wrbuf = uv_buf_init(response, strlen(response));
        uv_write(write_req, work->client, &wrbuf, 1, on_write);
        uv_close((uv_handle_t*) work->client, NULL);

        free(work->data);
        free(work->url_path);
        free(work->response_data);
        free(work);
    }
}

void work_cb(uv_work_t* req) {
    app_sched_t* work = (app_sched_t*) req->data;

    if (work->method == HTTP_POST) {
        proc_post_request(work);
    } else if (work->method == HTTP_GET) {
        proc_get_request(work);
    }
}

int on_url(http_parser* parser, const char* at, size_t length) {
    http_requests_t* client = (http_requests_t*) parser->data;
    strncpy(client->url_path, at, length);
    client->url_path[length] = '\0';
    return 0;
}

int on_header_field(http_parser* parser, const char* at, size_t length) {
    http_requests_t* client = (http_requests_t*) parser->data;
    client->header_type = strncasecmp(at, "Content-Type", length) == 0;
    return 0;
}

int on_header_value(http_parser* parser, const char* at, size_t length) {
    http_requests_t* client = (http_requests_t*) parser->data;
    if (client->header_type) {
        if (strncasecmp(at, "application/json", length) == 0) {
            client->content_type_json = 1;
        } else {
            client->content_type_json = 0;
        }
    }
    return 0;
}

int on_body(http_parser* parser, const char* at, size_t length) {
    http_requests_t* client = (http_requests_t*) parser->data;
    client->body = realloc(client->body, client->body_length + length + 1);
    memcpy(client->body + client->body_length, at, length);
    client->body_length += length;
    client->body[client->body_length] = '\0';
    return 0;
}

int on_message_complete(http_parser* parser) {
    http_requests_t* client = (http_requests_t*) parser->data;

    app_sched_t* work = (app_sched_t*) malloc(sizeof(app_sched_t));
    work->req.data = (void*) work;
    work->client = (uv_stream_t*) &client->handle;
    work->data = client->body ? strdup(client->body) : NULL;
    work->url_path = strdup(client->url_path);
    work->response_data = NULL;
    work->response_length = 0;
    work->method = client->parser.method;

    if (client->parser.method == HTTP_POST && !client->content_type_json) {
        uv_write_t* write_req = (uv_write_t*) malloc(sizeof(uv_write_t));
        uv_buf_t wrbuf = uv_buf_init(RESPONSE_BAD_REQUEST, strlen(RESPONSE_BAD_REQUEST));
        uv_write(write_req, (uv_stream_t*) &client->handle, &wrbuf, 1, on_write);
        uv_close((uv_handle_t*) &client->handle, NULL);
        free(work);
    } else {
        uv_queue_work(uv_default_loop(), &work->req, work_cb, after_work_cb);
    }

    free(client->body);
    client->body = NULL;
    client->body_length = 0;

    return 0;
}

void on_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
    http_requests_t* client = (http_requests_t*) stream->data;

    if (nread > 0) {
        size_t parsed = http_parser_execute(&client->parser, &client->settings, buf->base, nread);
        if (parsed < nread) {
            LOGE("HTTP parse error: %s", http_errno_description(HTTP_PARSER_ERRNO(&client->parser)));
            uv_close((uv_handle_t*) &client->handle, NULL);
        }
    } else if (nread < 0) {
        if (nread != UV_EOF) {
            LOGE("Read error %s", uv_err_name(nread));
        }
        uv_close((uv_handle_t*) &client->handle, NULL);
    }

    if (buf->base) {
        free(buf->base);
    }
}

void on_new_connection(uv_stream_t* server, int status) {
    if (status < 0) {
        LOGE("New connection error %s", uv_strerror(status));
        return;
    }

    http_requests_t* client = (http_requests_t*) malloc(sizeof(http_requests_t));
    uv_tcp_init(uv_default_loop(), &client->handle);
    client->handle.data = client;

    http_parser_init(&client->parser, HTTP_REQUEST);
    client->parser.data = client;

    client->body = NULL;
    client->body_length = 0;
    client->content_type_json = 0;
    client->header_type = 0;
    client->settings.on_url = on_url;
    client->settings.on_header_field = on_header_field;
    client->settings.on_header_value = on_header_value;
    client->settings.on_body = on_body;
    client->settings.on_message_complete = on_message_complete;

    if (uv_accept(server, (uv_stream_t*) &client->handle) == 0) {
        uv_read_start((uv_stream_t*) &client->handle, alloc_buffer, on_read);
    } else {
        uv_close((uv_handle_t*) &client->handle, NULL);
    }
}