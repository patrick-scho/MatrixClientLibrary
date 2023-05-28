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
    
    char server[SERVER_SIZE]; int serverLen;
    char accessTokenBuffer[ACCESS_TOKEN_SIZE]; int accessTokenLen;
    char deviceIdBuffer[DEVICE_ID_SIZE]; int deviceIdLen;
    char expireMsBuffer[EXPIRE_MS_SIZE]; int expireMsLen;
    char refreshTokenBuffer[REFRESH_TOKEN_SIZE]; int refreshTokenLen;

    void * httpUserData;
} MatrixClient;

bool
MatrixClientInit(
    MatrixClient * client,
    char * server, int serverLen);

bool
MatrixClientLoginPassword(
    MatrixClient * client,
    char * username, int usernameLen,
    char * password, int passwordLen,
    char * displayName, int displayNameLen);

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
    char * outResponseBuffer, int outResponseCap, int * outResponseLen);

bool
MatrixHttpPost(
    MatrixClient * client,
    const char * url,
    char * requestBuffer, int requestLen,
    char * outResponseBuffer, int outResponseCap, int * outResponseLen);

#endif
