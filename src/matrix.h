#ifndef MATRIX__H
#define MATRIX__H

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <olm/olm.h>



#define USER_ID_SIZE 64
#define SERVER_SIZE 20
#define ACCESS_TOKEN_SIZE 40
#define DEVICE_ID_SIZE 20
#define EXPIRE_MS_SIZE 20
#define REFRESH_TOKEN_SIZE 20
#define MAX_URL_LEN 128

#define OLM_IDENTITY_KEYS_JSON_SIZE 128
#define DEVICE_KEY_SIZE 44
#define SIGNING_KEY_SIZE 44

#define KEY_SHARE_EVENT_LEN 1024

#define OLM_ACCOUNT_MEMORY_SIZE 7528
#define OLM_ACCOUNT_RANDOM_SIZE (32+32)

#define OLM_SESSION_MEMORY_SIZE 3352
#define OLM_ENCRYPT_RANDOM_SIZE 32

#define OLM_ONETIME_KEYS_RANDOM_SIZE 32*10
#define OLM_KEY_ID_SIZE 32

#define OLM_SIGNATURE_SIZE 128

#define MEGOLM_OUTBOUND_SESSION_MEMORY_SIZE 232
#define MEGOLM_SESSION_ID_SIZE 44
#define MEGOLM_SESSION_KEY_SIZE 306
#define MEGOLM_INIT_RANDOM_SIZE (4*32 + 32)

#define JSON_ONETIME_KEY_SIZE 128
#define JSON_ONETIME_KEY_SIGNED_SIZE 256
#define JSON_SIGNATURE_SIZE 256

#define NUM_MEGOLM_SESSIONS 10
#define NUM_OLM_SESSIONS 10
#define NUM_DEVICES 10

void
Randomize(uint8_t * random, int randomLen);

bool
JsonEscape(
    char * sIn, int sInLen,
    char * sOut, int sOutCap);
    
bool JsonSign(
    char * sIn, int sInLen,
    char * sOut, int sOutCap);

// Matrix Device

typedef struct MatrixDevice {
    char deviceId[DEVICE_ID_SIZE];
    char deviceKey[DEVICE_KEY_SIZE];
} MatrixDevice;


// Matrix Olm Account

typedef struct MatrixOlmAccount {
    OlmAccount * account;
    char memory[OLM_ACCOUNT_MEMORY_SIZE];
} MatrixOlmAccount;

bool
MatrixOlmAccountInit(
    MatrixOlmAccount * account);


// Matrix Olm Session

typedef struct MatrixOlmSession {
    const char * deviceId;

    int type;
    OlmSession * session;
    char memory[OLM_SESSION_MEMORY_SIZE];
} MatrixOlmSession;

bool
MatrixOlmSessionInit(
    MatrixOlmSession * session,
    const char * deviceId);

bool
MatrixOlmSessionEncrypt(
    MatrixOlmSession * session,
    const char * plaintext,
    char * outBuffer, int outBufferCap);


// Matrix Megolm Session

typedef struct MatrixMegolmInSession {
    OlmInboundGroupSession * session;
} MatrixMegolmInSession;

typedef struct MatrixMegolmOutSession {
    const char * roomId;

    OlmOutboundGroupSession * session;
    char memory[MEGOLM_OUTBOUND_SESSION_MEMORY_SIZE];

    char id[MEGOLM_SESSION_ID_SIZE];
    char key[MEGOLM_SESSION_KEY_SIZE];
} MatrixMegolmOutSession;

bool
MatrixMegolmOutSessionInit(
    MatrixMegolmOutSession * session,
    const char * roomId);
    
bool
MatrixMegolmOutSessionEncrypt(
    MatrixMegolmOutSession * session,
    const char * plaintext,
    char * outBuffer, int outBufferCap);


// Matrix Client

typedef struct MatrixClient {
    MatrixOlmAccount olmAccount;

    MatrixMegolmInSession megolmInSessions[NUM_MEGOLM_SESSIONS];
    int numMegolmInSessions;
    MatrixMegolmOutSession megolmOutSessions[NUM_MEGOLM_SESSIONS];
    int numMegolmOutSessions;
    MatrixOlmSession olmSessions[NUM_OLM_SESSIONS];
    int numOlmSessions;
    
    MatrixDevice devices[NUM_DEVICES];
    int numDevices;
    
    char deviceKey[DEVICE_KEY_SIZE];
    char signingKey[DEVICE_KEY_SIZE];

    char userId[USER_ID_SIZE];
    char server[SERVER_SIZE];
    char accessToken[ACCESS_TOKEN_SIZE];
    char deviceId[DEVICE_ID_SIZE];
    char expireMs[EXPIRE_MS_SIZE];
    char refreshToken[REFRESH_TOKEN_SIZE];

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
MatrixClientSetDeviceId(
    MatrixClient * client,
    const char * deviceId);

bool
MatrixClientSetUserId(
    MatrixClient * client,
    const char * userId);

bool
MatrixClientGenerateOnetimeKeys(
    MatrixClient * client,
    int numberOfKeys);

bool
MatrixClientUploadOnetimeKeys(
    MatrixClient * client);

bool
MatrixClientUploadDeviceKeys(
    MatrixClient * client);

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
MatrixClientSendEventEncrypted(
    MatrixClient * client,
    const char * roomId,
    const char * msgType,
    const char * msgBody);

bool
MatrixClientSync(
    MatrixClient * client,
    char * outSyncBuffer, int outSyncCap);

bool
MatrixClientShareMegolmOutSession(
    MatrixClient * client,
    const char * deviceId,
    MatrixMegolmOutSession * session);

bool
MatrixClientShareMegolmOutSessionTest(
    MatrixClient * client,
    const char * deviceId,
    MatrixMegolmOutSession * session);

bool
MatrixClientGetMegolmOutSession(
    MatrixClient * client,
    const char * roomId,
    MatrixMegolmOutSession ** outSession);

bool
MatrixClientSetMegolmOutSession(
    MatrixClient * client,
    const char * roomId,
    MatrixMegolmOutSession session);

bool
MatrixClientGetOlmSession(
    MatrixClient * client,
    const char * deviceId,
    MatrixOlmSession ** outSession);

bool
MatrixClientSendToDevice(
    MatrixClient * client,
    const char * userId,
    const char * deviceId,
    const char * message,
    const char * msgType);

bool
MatrixClientSendToDeviceEncrypted(
    MatrixClient * client,
    const char * userId,
    const char * deviceId,
    const char * message,
    const char * msgType);

bool
MatrixClientGetDeviceKey(
    MatrixClient * client,
    const char * deviceId,
    char * outDeviceKey, int outDeviceKeyCap);

bool
MatrixClientGetDeviceKey(
    MatrixClient * client,
    const char * deviceId,
    char * outDeviceKey, int outDeviceKeyCap);

bool
MatrixClientRequestDeviceKeys(
    MatrixClient * client);



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
