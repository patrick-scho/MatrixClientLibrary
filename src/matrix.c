#include "matrix.h"

#include <time.h>
#include <stdio.h>
#include <mjson.h>


#define LOGIN_REQUEST_SIZE 1024
#define LOGIN_RESPONSE_SIZE 1024
#define LOGIN_URL "/_matrix/client/v3/login"

#define ROOMEVENT_REQUEST_SIZE 1024
#define ROOMEVENT_RESPONSE_SIZE 1024
#define ROOMEVENT_URL "/_matrix/client/v3/rooms/%s/send/%s/%d"


bool
MatrixClientInit(
    MatrixClient * client,
    const char * server)
{
    strcpy_s(
        client->server,
        SERVER_SIZE,
        server
    );

    return true;
}

bool
MatrixClientSetAccessToken(
    MatrixClient * client,
    const char * accessToken)
{
    int accessTokenLen = strlen(accessToken);

    if (accessTokenLen < ACCESS_TOKEN_SIZE - 1)
        return false;

    for (int i = 0; i < accessTokenLen; i++)
        client->accessTokenBuffer[i] = accessToken[i];

    return true;
}

// https://spec.matrix.org/v1.6/client-server-api/#post_matrixclientv3login
bool
MatrixClientLoginPassword(
    MatrixClient * client,
    const char * username,
    const char * password,
    const char * displayName)
{
    static char requestBuffer[LOGIN_REQUEST_SIZE];

    mjson_snprintf(requestBuffer, LOGIN_REQUEST_SIZE,
        "{"
            "\"type\": \"m.login.password\","
            "\"identifier\": {"
                "\"type\": \"m.id.user\","
                "\"user\": \"%s\""
            "},"
            "\"password\": \"%s\","
            "\"initial_device_display_name\": \"%s\""
        "}",
        username,
        password,
        displayName);
    
    static char responseBuffer[LOGIN_RESPONSE_SIZE];
    bool result =
        MatrixHttpPost(client,
            LOGIN_URL,
            requestBuffer,
            responseBuffer, LOGIN_RESPONSE_SIZE,
            false);
    
    int responseLen = strlen(responseBuffer);
    
    if (!result)
        return false;

    mjson_get_string(responseBuffer, responseLen,
        "$.access_token",
        client->accessTokenBuffer, ACCESS_TOKEN_SIZE);
    mjson_get_string(responseBuffer, responseLen,
        "$.device_id",
        client->deviceIdBuffer, DEVICE_ID_SIZE);
    mjson_get_string(responseBuffer, responseLen,
        "$.expires_in_ms",
        client->expireMsBuffer, EXPIRE_MS_SIZE);
    mjson_get_string(responseBuffer, responseLen,
        "$.refresh_token",
        client->refreshTokenBuffer, REFRESH_TOKEN_SIZE);

    return true;
}
    
bool
MatrixClientSendEvent(
    MatrixClient * client,
    const char * roomId,
    const char * msgType,
    const char * msgBody)
{    
    static char requestUrl[MAX_URL_LEN];
    sprintf_s(requestUrl, MAX_URL_LEN,
        ROOMEVENT_URL, roomId, msgType, time(NULL));

    static char responseBuffer[ROOMEVENT_RESPONSE_SIZE];
    bool result =
        MatrixHttpPut(client,
            requestUrl,
            msgBody,
            responseBuffer, ROOMEVENT_RESPONSE_SIZE,
            true);
    
    return result;
}

