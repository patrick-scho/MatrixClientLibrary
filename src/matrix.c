#include "matrix.h"

#include <stdio.h>
#include <mjson.h>


#define LOGIN_REQUEST_SIZE 1024
#define LOGIN_RESPONSE_SIZE 1024
#define LOGIN_URL "/_matrix/client/v3/login"


bool
MatrixClientInit(
    MatrixClient * client,
    char * server, int serverLen)
{
    strcpy_s(
        client->server,
        SERVER_SIZE,
        server
    );
    client->serverLen = serverLen;

    return true;
}

// https://spec.matrix.org/v1.6/client-server-api/#post_matrixclientv3login
bool
MatrixClientLoginPassword(
    MatrixClient * client,
    char * username, int usernameLen,
    char * password, int passwordLen,
    char * displayName, int displayNameLen)
{
    static char requestBuffer[LOGIN_REQUEST_SIZE];

    int requestLen =
        mjson_snprintf(requestBuffer, LOGIN_REQUEST_SIZE,
            "{"
                "\"type\": \"m.login.password\","
                "\"identifier\": {"
                    "\"type\": \"m.id.user\","
                    "\"user\": \"%.*s\""
                "},"
                "\"password\": \"%.*s\","
                "\"initial_device_display_name\": \"%.*s\""
            "}",
            usernameLen, username,
            passwordLen, password,
            displayNameLen, displayName);
    
    static char responseBuffer[LOGIN_RESPONSE_SIZE];
    int responseLen;
    bool result =
        MatrixHttpPost(client,
            LOGIN_URL,
            requestBuffer, requestLen,
            responseBuffer, LOGIN_RESPONSE_SIZE, &responseLen);
    
    if (!result)
        return false;

    client->accessTokenLen =
        mjson_get_string(responseBuffer, responseLen,
            "$.access_token",
            client->accessTokenBuffer, ACCESS_TOKEN_SIZE);
    client->deviceIdLen =
        mjson_get_string(responseBuffer, responseLen,
            "$.device_id",
            client->deviceIdBuffer, DEVICE_ID_SIZE);
    client->expireMsLen =
        mjson_get_string(responseBuffer, responseLen,
            "$.expires_in_ms",
            client->expireMsBuffer, EXPIRE_MS_SIZE);
    client->refreshTokenLen =
        mjson_get_string(responseBuffer, responseLen,
            "$.refresh_token",
            client->refreshTokenBuffer, REFRESH_TOKEN_SIZE);

    return true;
}

