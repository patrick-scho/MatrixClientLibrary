#ifndef MATRIX__H
#define MATRIX__H

#include <stdbool.h>
#include <string.h>

#include <olm/olm.h>



// TODO: fix
#define SERVER_SIZE 20
#define ACCESS_TOKEN_SIZE 40
#define DEVICE_ID_SIZE 20
#define EXPIRE_MS_SIZE 20
#define REFRESH_TOKEN_SIZE 20
#define MAX_URL_LEN 128


typedef struct MatrixClient {
    OlmAccount * olmAccount;
    OlmSession * olmSession;
    
    char server[SERVER_SIZE+1];
    char accessTokenBuffer[ACCESS_TOKEN_SIZE];
    char deviceIdBuffer[DEVICE_ID_SIZE];
    char expireMsBuffer[EXPIRE_MS_SIZE];
    char refreshTokenBuffer[REFRESH_TOKEN_SIZE];

    void * httpUserData;
} MatrixClient;

bool
MatrixClientInit(
    MatrixClient * client,
    const char * server);

bool
MatrixClientSetAccessToken(
    MatrixClient * client,
    const char * accessToken);

bool
MatrixClientLoginPassword(
    MatrixClient * client,
    const char * username,
    const char * password,
    const char * displayName);
    
bool
MatrixClientSendEvent(
    MatrixClient * client,
    const char * roomId,
    const char * msgType,
    const char * msgBody);

bool
MatrixHttpInit(
    MatrixClient * client);

bool
MatrixHttpDeinit(
    MatrixClient * client);

bool
MatrixHttpGet(
    MatrixClient * client,
    const char * url,
    char * outResponseBuffer, int outResponseCap,
    bool authenticated);

bool
MatrixHttpPost(
    MatrixClient * client,
    const char * url,
    const char * requestBuffer,
    char * outResponseBuffer, int outResponseCap,
    bool authenticated);

bool
MatrixHttpPut(
    MatrixClient * client,
    const char * url,
    const char * requestBuffer,
    char * outResponseBuffer, int outResponseCap,
    bool authenticated);

#endif
