#include "matrix.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <mongoose.h>

//#define HTTP_DATA_SIZE 1024

typedef struct MatrixHttpConnection {
    struct mg_mgr mgr;
    struct mg_connection * connection;
    bool connected;
    char * data;
    int dataCap;
    int dataLen;
    bool dataReceived;
} MatrixHttpConnection;

static void
MatrixHttpCallback(
    struct mg_connection *c,
    int ev,
    void *ev_data,
    void *fn_data)
{
    MatrixClient * client = (MatrixClient *)fn_data;
    MatrixHttpConnection * conn = (MatrixHttpConnection *)client->httpUserData;

    if (ev == MG_EV_CONNECT)
    {
        struct mg_str host = mg_url_host(client->server);

        // If s_url is https://, tell client connection to use TLS
        if (mg_url_is_ssl(client->server))
        {
            struct mg_tls_opts opts = {.srvname = host};
            mg_tls_init(c, &opts);
        }

        conn->connection = c;
        conn->connected = true;
    }
    if (ev == MG_EV_HTTP_MSG)
    {
        // Response
        struct mg_http_message *hm = (struct mg_http_message *)ev_data;
        // memcpy_s(client->data, 1024, hm->message.ptr, hm->message.len);
        // client->dataLen = hm->message.len;
        memcpy_s(conn->data, conn->dataCap, hm->body.ptr, hm->body.len);
        conn->dataLen = hm->body.len;
        conn->dataReceived = true;
    }
}

bool
MatrixHttpInit(
    MatrixClient * client)
{
    MatrixHttpConnection * conn =
        (MatrixHttpConnection *)malloc(sizeof(MatrixHttpConnection));

    client->httpUserData = conn;
    
    mg_mgr_init(&conn->mgr);

    mg_http_connect(&conn->mgr, client->server, MatrixHttpCallback, client);

    while (! conn->connected)
        mg_mgr_poll(&conn->mgr, 1000);

    return conn->connected;
}

bool
MatrixHttpDeinit(
    MatrixClient * client)
{
    MatrixHttpConnection * conn = (MatrixHttpConnection *)client->httpUserData;
    
    mg_mgr_free(&conn->mgr);

    free(conn);

    return true;
}

bool
MatrixHttpGet(
    MatrixClient * client,
    const char * url,
    char * outResponseBuffer, int outResponseCap)
{
    MatrixHttpConnection * conn = (MatrixHttpConnection *)client->httpUserData;

    conn->dataReceived = false;

    struct mg_str host = mg_url_host(client->server);

    mg_printf(conn->connection,
        "GET %s HTTP/1.1\r\n"
        "Host: %.*s\r\n"
        "\r\n",
        url,
        host.len, host.ptr);

    conn->data = outResponseBuffer;
    conn->dataCap = outResponseCap;
    
    while (! conn->dataReceived)
        mg_mgr_poll(&conn->mgr, 1000);

    return conn->dataReceived;
}

bool
MatrixHttpPost(
    MatrixClient * client,
    const char * url,
    const char * requestBuffer,
    char * outResponseBuffer, int outResponseCap)
{
    MatrixHttpConnection * conn = (MatrixHttpConnection *)client->httpUserData;

    conn->dataReceived = false;

    struct mg_str host = mg_url_host(client->server);

    mg_printf(conn->connection,
            "POST %s HTTP/1.0\r\n"
            "Host: %.*s\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: %d\r\n"
            "\r\n"
            "%s"
            "\r\n",
            url,
            host.len, host.ptr,
            strlen(requestBuffer),
            requestBuffer);

    conn->data = outResponseBuffer;
    conn->dataCap = outResponseCap;
    
    while (! conn->dataReceived)
        mg_mgr_poll(&conn->mgr, 1000);

    return conn->dataReceived;
}