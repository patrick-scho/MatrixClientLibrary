#include "matrix.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <mongoose.h>

#define AUTHORIZATION_HEADER_LEN 64

typedef struct MatrixHttpConnection {
    struct mg_mgr mgr;
    struct mg_connection * connection;
    
    const char * host;
    const char * accessToken;

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
    MatrixHttpConnection * conn = (MatrixHttpConnection *)fn_data;

    if (ev == MG_EV_CONNECT)
    {
        struct mg_str host = mg_url_host(conn->host);

        // If s_url is https://, tell client connection to use TLS
        if (mg_url_is_ssl(conn->host))
        {
            static struct mg_tls_opts opts;
            opts.srvname = host;
            mg_tls_init(c, &opts);
        }

        conn->connection = c;
        conn->connected = true;
    }
    if (ev == MG_EV_HTTP_CHUNK)
    {
        
    }
    if (ev == MG_EV_HTTP_MSG)
    {
        // Response
        struct mg_http_message *hm = (struct mg_http_message *)ev_data;
        
        memcpy(conn->data, hm->body.ptr, hm->body.len);
        
        conn->data[hm->body.len] = '\0';
        conn->dataLen = hm->body.len;
        conn->dataReceived = true;
    }
    if (ev == MG_EV_CLOSE)
    {
        conn->connection = NULL;
        conn->connected = false;
    }
}

bool
MatrixHttpConnect(
    MatrixHttpConnection * hc)
{    
    //struct mg_connection * c =
        mg_http_connect(&hc->mgr, hc->host, MatrixHttpCallback, hc);

    while (! hc->connected)
        mg_mgr_poll(&hc->mgr, 1000);

    return hc->connected;
}

bool
MatrixHttpInit(
    MatrixHttpConnection ** hc,
    const char * host)
{
    *hc = (MatrixHttpConnection *)calloc(1, sizeof(MatrixHttpConnection));
    
    (*hc)->host = host;
    
    mg_mgr_init(&(*hc)->mgr);

    MatrixHttpConnect(*hc);

    return true;
}

bool
MatrixHttpDeinit(
    MatrixHttpConnection ** hc)
{
    mg_mgr_free(&(*hc)->mgr);

    free(*hc);
    *hc = NULL;

    return true;
}

bool
MatrixHttpSetAccessToken(
    MatrixHttpConnection * hc,
    const char * accessToken)
{
    hc->accessToken = accessToken;

    return true;
}

bool
MatrixHttpGet(
    MatrixHttpConnection * hc,
    const char * url,
    char * outResponseBuffer, int outResponseCap,
    bool authenticated)
{
    if (! hc->connected)
        MatrixHttpConnect(hc);

    hc->dataReceived = false;

    struct mg_str host = mg_url_host(hc->host);

    static char authorizationHeader[AUTHORIZATION_HEADER_LEN];
    if (authenticated)
        snprintf(authorizationHeader, AUTHORIZATION_HEADER_LEN,
            "Authorization: Bearer %s\r\n", hc->accessToken);
    else
        authorizationHeader[0] = '\0';

    mg_printf(hc->connection,
        "GET %s HTTP/1.1\r\n"
        "Host: %.*s\r\n"
        "%s"
        "\r\n",
        url,
        host.len, host.ptr,
        authorizationHeader);

    hc->data = outResponseBuffer;
    hc->dataCap = outResponseCap;
    
    while (! hc->dataReceived)
        mg_mgr_poll(&hc->mgr, 1000);

    return hc->dataReceived;
}

bool
MatrixHttpPost(
    MatrixHttpConnection * hc,
    const char * url,
    const char * requestBuffer,
    char * outResponseBuffer, int outResponseCap,
    bool authenticated)
{
    if (! hc->connected)
        MatrixHttpConnect(hc);

    hc->dataReceived = false;

    struct mg_str host = mg_url_host(hc->host);

    static char authorizationHeader[AUTHORIZATION_HEADER_LEN];
    if (authenticated)
        snprintf(authorizationHeader, AUTHORIZATION_HEADER_LEN,
            "Authorization: Bearer %s\r\n", hc->accessToken);
    else
        authorizationHeader[0] = '\0';

    mg_printf(hc->connection,
            "POST %s HTTP/1.0\r\n"
            "Host: %.*s\r\n"
            "%s"
            "Content-Type: application/json\r\n"
            "Content-Length: %d\r\n"
            "\r\n"
            "%s"
            "\r\n",
            url,
            (int)host.len, host.ptr,
            authorizationHeader,
            (int)strlen(requestBuffer),
            requestBuffer);

    hc->data = outResponseBuffer;
    hc->dataCap = outResponseCap;
    
    while (! hc->dataReceived)
        mg_mgr_poll(&hc->mgr, 1000);

    return hc->dataReceived;
}

bool
MatrixHttpPut(
    MatrixHttpConnection * hc,
    const char * url,
    const char * requestBuffer,
    char * outResponseBuffer, int outResponseCap,
    bool authenticated)
{
    if (! hc->connected)
        MatrixHttpConnect(hc);

    hc->dataReceived = false;

    struct mg_str host = mg_url_host(hc->host);

    static char authorizationHeader[AUTHORIZATION_HEADER_LEN];
    if (authenticated)
        snprintf(authorizationHeader, AUTHORIZATION_HEADER_LEN,
            "Authorization: Bearer %s\r\n", hc->accessToken);
    else
        authorizationHeader[0] = '\0';

    mg_printf(hc->connection,
            "PUT %s HTTP/1.0\r\n"
            "Host: %.*s\r\n"
            "%s"
            "Content-Type: application/json\r\n"
            "Content-Length: %d\r\n"
            "\r\n"
            "%s"
            "\r\n",
            url,
            (int)host.len, host.ptr,
            authorizationHeader,
            (int)strlen(requestBuffer),
            requestBuffer);

    hc->data = outResponseBuffer;
    hc->dataCap = outResponseCap;
    
    while (! hc->dataReceived)
        mg_mgr_poll(&hc->mgr, 1000);

    return hc->dataReceived;
}
