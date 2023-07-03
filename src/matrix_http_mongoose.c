#include "matrix.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <mongoose.h>

//#define HTTP_DATA_SIZE 1024
#define AUTHORIZATION_HEADER_LEN 64

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
            static struct mg_tls_opts opts;
            opts.srvname = host;
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
        memcpy(conn->data, hm->body.ptr, hm->body.len);
        // memcpy_s(conn->data, conn->dataCap, hm->body.ptr, hm->body.len);
        conn->data[hm->body.len] = '\0';
        conn->dataLen = hm->body.len;
        conn->dataReceived = true;

        printf("received[%d]:\n%.*s\n", conn->dataLen, conn->dataLen, conn->data);
    }
    if (ev == MG_EV_CLOSE)
    {
        conn->connection = NULL;
        conn->connected = false;
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

    return MatrixHttpConnect(client);
}

bool
MatrixHttpConnect(
    MatrixClient * client)
{
    MatrixHttpConnection * conn =
        (MatrixHttpConnection *)client->httpUserData;
    
    struct mg_connection * c =
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
    char * outResponseBuffer, int outResponseCap,
    bool authenticated)
{
    MatrixHttpConnection * conn = (MatrixHttpConnection *)client->httpUserData;
    if (! conn->connected)
        MatrixHttpConnect(client);

    conn->dataReceived = false;

    struct mg_str host = mg_url_host(client->server);

    static char authorizationHeader[AUTHORIZATION_HEADER_LEN];
    if (authenticated)
        sprintf(authorizationHeader,
            "Authorization: Bearer %s\r\n", client->accessToken);
        // sprintf_s(authorizationHeader, AUTHORIZATION_HEADER_LEN,
        //     "Authorization: Bearer %s\r\n", client->accessToken);
    else
        authorizationHeader[0] = '\0';

    mg_printf(conn->connection,
        "GET %s HTTP/1.1\r\n"
        "Host: %.*s\r\n"
        "%s"
        "\r\n",
        url,
        host.len, host.ptr,
        authorizationHeader);

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
    char * outResponseBuffer, int outResponseCap,
    bool authenticated)
{
    MatrixHttpConnection * conn = (MatrixHttpConnection *)client->httpUserData;
    if (! conn->connected)
        MatrixHttpConnect(client);

    conn->dataReceived = false;

    struct mg_str host = mg_url_host(client->server);

    static char authorizationHeader[AUTHORIZATION_HEADER_LEN];
    if (authenticated)
        sprintf(authorizationHeader,
            "Authorization: Bearer %s\r\n", client->accessToken);
    else
        authorizationHeader[0] = '\0';

    mg_printf(conn->connection,
            "POST %s HTTP/1.0\r\n"
            "Host: %.*s\r\n"
            "%s"
            "Content-Type: application/json\r\n"
            "Content-Length: %d\r\n"
            "\r\n"
            "%s"
            "\r\n",
            url,
            host.len, host.ptr,
            authorizationHeader,
            strlen(requestBuffer),
            requestBuffer);

    conn->data = outResponseBuffer;
    conn->dataCap = outResponseCap;
    
    while (! conn->dataReceived)
        mg_mgr_poll(&conn->mgr, 1000);

    return conn->dataReceived;
}

bool
MatrixHttpPut(
    MatrixClient * client,
    const char * url,
    const char * requestBuffer,
    char * outResponseBuffer, int outResponseCap,
    bool authenticated)
{
    MatrixHttpConnection * conn = (MatrixHttpConnection *)client->httpUserData;
    if (! conn->connected)
        MatrixHttpConnect(client);

    conn->dataReceived = false;

    struct mg_str host = mg_url_host(client->server);

    static char authorizationHeader[AUTHORIZATION_HEADER_LEN];
    if (authenticated)
        sprintf(authorizationHeader,
            "Authorization: Bearer %s\r\n", client->accessToken);
    else
        authorizationHeader[0] = '\0';

    printf("PUT %s HTTP/1.0\r\n"
            "Host: %.*s\r\n"
            "%s"
            "Content-Type: application/json\r\n"
            "Content-Length: %d\r\n"
            "\r\n"
            "%s"
            "\r\n",
            url,
            host.len, host.ptr,
            authorizationHeader,
            strlen(requestBuffer),
            requestBuffer);

    mg_printf(conn->connection,
            "PUT %s HTTP/1.0\r\n"
            "Host: %.*s\r\n"
            "%s"
            "Content-Type: application/json\r\n"
            "Content-Length: %d\r\n"
            "\r\n"
            "%s"
            "\r\n",
            url,
            host.len, host.ptr,
            authorizationHeader,
            strlen(requestBuffer),
            requestBuffer);

    conn->data = outResponseBuffer;
    conn->dataCap = outResponseCap;
    
    while (! conn->dataReceived)
        mg_mgr_poll(&conn->mgr, 1000);

    return conn->dataReceived;
}
