#ifndef MATRIX__H
#define MATRIX__H

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <olm/olm.h>



#define USER_ID_SIZE 64
#define ROOM_ID_SIZE 128
#define SERVER_SIZE 20
#define ACCESS_TOKEN_SIZE 40
#define DEVICE_ID_SIZE 20
#define EXPIRE_MS_SIZE 20
#define REFRESH_TOKEN_SIZE 20
#define MAX_URL_LEN 1024

#define OLM_IDENTITY_KEYS_JSON_SIZE 128
#define DEVICE_KEY_SIZE 44
#define SIGNING_KEY_SIZE 44
#define ONETIME_KEY_SIZE 44
#define MASTER_KEY_SIZE 44

#define KEY_SHARE_EVENT_LEN 1024

#define OLM_ACCOUNT_MEMORY_SIZE 7528
#define OLM_ACCOUNT_RANDOM_SIZE (32+32)

#define OLM_SESSION_MEMORY_SIZE 3352
#define OLM_ENCRYPT_RANDOM_SIZE 32
#define OLM_OUTBOUND_SESSION_RANDOM_SIZE (32*2)

#define OLM_ONETIME_KEYS_RANDOM_SIZE (32*10)
#define OLM_KEY_ID_SIZE 32

#define OLM_SIGNATURE_SIZE 128

#define MEGOLM_OUTBOUND_SESSION_MEMORY_SIZE 232
#define MEGOLM_SESSION_ID_SIZE 44
#define MEGOLM_SESSION_KEY_SIZE 306
#define MEGOLM_INIT_RANDOM_SIZE (4*32 + 32)

#define JSON_ONETIME_KEY_SIZE 128
#define JSON_ONETIME_KEY_SIGNED_SIZE 256
#define JSON_SIGNATURE_SIZE 256

#define NUM_MEGOLM_SESSIONS 2
#define NUM_OLM_SESSIONS 2
#define NUM_DEVICES 10

// HTTP

typedef struct MatrixHttpConnection MatrixHttpConnection;

bool
MatrixHttpInit(
    MatrixHttpConnection ** hc,
    const char * host);

bool
MatrixHttpDeinit(
    MatrixHttpConnection ** hc);
    
bool
MatrixHttpSetAccessToken(
    MatrixHttpConnection * hc,
    const char * accessToken);

bool
MatrixHttpGet(
    MatrixHttpConnection * hc,
    const char * url,
    char * outResponseBuffer, int outResponseCap,
    bool authenticated);

bool
MatrixHttpPost(
    MatrixHttpConnection * hc,
    const char * url,
    const char * requestBuffer,
    char * outResponseBuffer, int outResponseCap,
    bool authenticated);

bool
MatrixHttpPut(
    MatrixHttpConnection * hc,
    const char * url,
    const char * requestBuffer,
    char * outResponseBuffer, int outResponseCap,
    bool authenticated);



// Matrix Device

typedef struct MatrixDevice {
    char deviceId[DEVICE_ID_SIZE];
    char deviceKey[DEVICE_KEY_SIZE];
    char signingKey[SIGNING_KEY_SIZE];
} MatrixDevice;


// Matrix Olm Account

typedef struct MatrixOlmAccount {
    OlmAccount * account;
    char memory[OLM_ACCOUNT_MEMORY_SIZE];
} MatrixOlmAccount;

bool
MatrixOlmAccountInit(
    MatrixOlmAccount * account);

bool
MatrixOlmAccountUnpickle(
    MatrixOlmAccount * account,
    void * pickled, int pickledLen,
    const void * key, int keyLen);

bool
MatrixOlmAccountGetDeviceKey(
    MatrixOlmAccount * account,
    char * key, int keyCap);
    
bool
MatrixOlmAccountGetSigningKey(
    MatrixOlmAccount * account,
    char * key, int keyCap);


// Matrix Olm Session

typedef struct MatrixOlmSession {
    const char * deviceId; // TODO: char[]

    int type;
    OlmSession * session;
    char memory[OLM_SESSION_MEMORY_SIZE];
} MatrixOlmSession;

bool
MatrixOlmSessionUnpickle(
    MatrixOlmSession * session,
    const char * deviceId,
    void * pickled, int pickledLen,
    const void * key, int keyLen);

// create an olm sesseion from a type 0 message
bool
MatrixOlmSessionFrom(
    MatrixOlmSession * session,
    OlmAccount * olmAccount,
    const char * deviceId,
    const char * deviceKey,
    const char * encrypted);

// create a new olm session from a claimed onetime key
bool
MatrixOlmSessionTo(
    MatrixOlmSession * session,
    OlmAccount * olmAccount,
    const char * deviceId,
    const char * deviceKey,
    const char * deviceOnetimeKey);

bool
MatrixOlmSessionEncrypt(
    MatrixOlmSession * session,
    const char * plaintext,
    char * outBuffer, int outBufferCap);

bool
MatrixOlmSessionDecrypt(
    MatrixOlmSession * session,
    size_t messageType,
    char * encrypted,
    char * outBuffer, int outBufferCap);


// Matrix Megolm Session

typedef struct MatrixMegolmInSession {
    char roomId[ROOM_ID_SIZE];
    char id[MEGOLM_SESSION_ID_SIZE];
    char key[MEGOLM_SESSION_KEY_SIZE];

    OlmInboundGroupSession * session;
    char memory[MEGOLM_OUTBOUND_SESSION_MEMORY_SIZE];

} MatrixMegolmInSession;

bool
MatrixMegolmInSessionInit(
    MatrixMegolmInSession * session,
    const char * roomId,
    const char * sessionId,
    const char * sessionKey, int sessionKeyLen);

bool
MatrixMegolmInSessionDecrypt(
    MatrixMegolmInSession * session,
    const char * encrypted, int encryptedLen,
    char * outDecrypted, int outDecryptedCap);

typedef struct MatrixMegolmOutSession {
    char roomId[ROOM_ID_SIZE];
    char id[MEGOLM_SESSION_ID_SIZE];
    char key[MEGOLM_SESSION_KEY_SIZE];

    OlmOutboundGroupSession * session;
    char memory[MEGOLM_OUTBOUND_SESSION_MEMORY_SIZE];
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

    char userId[USER_ID_SIZE];
    char accessToken[ACCESS_TOKEN_SIZE];
    char deviceId[DEVICE_ID_SIZE];
    char expireMs[EXPIRE_MS_SIZE];
    char refreshToken[REFRESH_TOKEN_SIZE];
    char masterKey[MASTER_KEY_SIZE];

    bool verified;

    MatrixHttpConnection * hc;
} MatrixClient;

bool
MatrixClientInit(
    MatrixClient * client);

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
MatrixClientClaimOnetimeKey(
    MatrixClient * client,
    const char * userId,
    const char * deviceId,
    char * outOnetimeKey, int outOnetimeKeyCap);

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
    char * outSyncBuffer, int outSyncCap,
    char * nextBatch, int nextBatchCap);

bool
MatrixClientGetRoomEvent(
    MatrixClient * client,
    const char * roomId,
    const char * eventId,
    char * outEvent, int outEventCap);

bool
MatrixClientShareMegolmOutSession(
    MatrixClient * client,
    const char * userId,
    const char * deviceId,
    MatrixMegolmOutSession * session);

// try to lookup outgoing megolm session, return true if found
bool
MatrixClientGetMegolmOutSession(
    MatrixClient * client,
    const char * roomId,
    MatrixMegolmOutSession ** outSession);

// create a new outgoing megolm session and store it locally
bool
MatrixClientNewMegolmOutSession(
    MatrixClient * client,
    const char * roomId,
    MatrixMegolmOutSession ** outSession);

// try to lookup incoming megolm session, return true if found
bool
MatrixClientGetMegolmInSession(
    MatrixClient * client,
    const char * roomId, int roomIdLen,
    const char * sessionId, int sessionIdLen,
    MatrixMegolmInSession ** outSession);

// create a new incoming megolm session and store it locally
bool
MatrixClientNewMegolmInSession(
    MatrixClient * client,
    const char * roomId,
    const char * sessionId,
    const char * sessionKey,
    MatrixMegolmInSession ** outSession);

// send a m.room_key_request to the device identified by userId/devideId
bool
MatrixClientRequestMegolmInSession(
    MatrixClient * client,
    const char * roomId,
    const char * sessionId,
    const char * senderKey,
    const char * userId,
    const char * deviceId);

// try to lookup olm session, return true if found
bool
MatrixClientGetOlmSession(
    MatrixClient * client,
    const char * userId,
    const char * deviceId,
    MatrixOlmSession ** outSession);

// create a new olm session from a type 0 message and store it locally
bool
MatrixClientNewOlmSessionIn(
    MatrixClient * client,
    const char * userId,
    const char * deviceId,
    const char * encrypted,
    MatrixOlmSession ** outSession);

// create a new olm session with device userId/deviceId and store it locally
// this automatically claims the onetime key
bool
MatrixClientNewOlmSessionOut(
    MatrixClient * client,
    const char * userId,
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
MatrixClientSendDummy(
    MatrixClient * client,
    const char * userId,
    const char * deviceId);

// lookup device key locally and if not present get it from server
bool
MatrixClientRequestDeviceKey(
    MatrixClient * client,
    const char * deviceId,
    char * outDeviceKey, int outDeviceKeyCap);
    
// lookup signing key locally and if not present get it from server
bool
MatrixClientRequestSigningKey(
    MatrixClient * client,
    const char * deviceId,
    char * outSigningKey, int outSigningKeyCap);

// lookup the master key for this user and if not present get it from server
bool
MatrixClientRequestMasterKey(
    MatrixClient * client,
    char * outMasterKey, int outMasterKeyCap);

// call keys/query and store retrieved information
// this is called by the other Request* functions
bool
MatrixClientRequestDeviceKeys(
    MatrixClient * client);

// delete this device on the server
bool
MatrixClientDeleteDevice(
    MatrixClient * client);


// util

void
Randomize(uint8_t * random, int randomLen);

bool
JsonEscape(
    const char * sIn, int sInLen,
    char * sOut, int sOutCap);
    
bool
JsonCanonicalize(
    const char * sIn, int sInLen,
    char * sOut, int sOutCap);
    
bool
JsonSign(
    MatrixClient * client,
    const char * sIn, int sInLen,
    char * sOut, int sOutCap);

#endif
