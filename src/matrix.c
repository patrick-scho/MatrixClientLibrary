#include "matrix.h"

#include <time.h>
#include <stdio.h>
#include <mjson.h>


#define LOGIN_REQUEST_SIZE 1024
#define LOGIN_RESPONSE_SIZE 1024
#define LOGIN_URL "/_matrix/client/v3/login"

#define ENCRYPTED_REQUEST_SIZE (1024*5)
#define ENCRYPTED_EVENT_SIZE (1024*10)
#define ROOMEVENT_REQUEST_SIZE 256
#define ROOMEVENT_RESPONSE_SIZE 1024
#define ROOMEVENT_URL "/_matrix/client/v3/rooms/%s/send/%s/%d"

#define TODEVICE_EVENT_SIZE (1024*5)
#define TODEVICE_URL "/_matrix/client/v3/sendToDevice/%s/%d"

#define KEYS_QUERY_URL "/_matrix/client/v3/keys/query"
#define KEYS_QUERY_REQUEST_SIZE 256
#define KEYS_QUERY_RESPONSE_SIZE (1024*10)

#define KEYS_UPLOAD_URL "/_matrix/client/v3/keys/upload"
#define KEYS_UPLOAD_REQUEST_SIZE 1024
#define KEYS_UPLOAD_REQUEST_SIGNED_SIZE 2048
#define KEYS_UPLOAD_RESPONSE_SIZE 2048

#define KEYS_CLAIM_URL "/_matrix/client/v3/keys/claim"
#define KEYS_CLAIM_REQUEST_SIZE 1024
#define KEYS_CLAIM_RESPONSE_SIZE 1024

#define JSON_QUERY_SIZE 128



void
Randomize(
    uint8_t * random,
    int randomLen)
{
    static bool first = true;
    if (first) { srand(time(0)); first = false; }

    for (int i = 0; i < randomLen; i++)
    {
        random[i] = rand() % 256;
    }
}

bool
JsonEscape(
    const char * sIn, int sInLen,
    char * sOut, int sOutCap)
{
    int sOutIndex = 0;

    for (int i = 0; i < sInLen; i++)
    {
        if (i >= sOutCap)
            return false;
        
        if (sIn[i] == '.' ||
            sIn[i] == '[' ||
            sIn[i] == ']'
        ) {
            sOut[sOutIndex++] = '\\';
        }
        sOut[sOutIndex++] = sIn[i];
    }

    if (sOutIndex < sOutCap)
        sOut[sOutIndex] = '\0';

    return true;
}

bool JsonSign(
    MatrixClient * client,
    const char * sIn, int sInLen,
    char * sOut, int sOutCap)
{
    static char signature[OLM_SIGNATURE_SIZE];
    size_t res =
        olm_account_sign(client->olmAccount.account,
            sIn, sInLen,
            signature, OLM_SIGNATURE_SIZE);
    
    int signatureLen = res;

    static char signatureJson[JSON_SIGNATURE_SIZE];
    int signatureJsonLen =
        mjson_snprintf(signatureJson, JSON_SIGNATURE_SIZE,
            "{"
                "\"signatures\":{"
                    "\"%s\":{"
                        "\"ed25519:%s\":\"%.*s\""
                    "}"
                "}"
            "}",
            client->userId,
            client->deviceId,
            signatureLen, signature);

    struct mjson_fixedbuf result = { sOut, sOutCap, 0 };
    mjson_merge(
        sIn, sInLen,
        signatureJson, signatureJsonLen,
        mjson_print_fixed_buf,
        &result);

    return true;
}


bool
MatrixOlmAccountInit(
    MatrixOlmAccount * account)
{
    account->account = olm_account(account->memory);

    static uint8_t random[OLM_ACCOUNT_RANDOM_SIZE];
    Randomize(random, OLM_ACCOUNT_RANDOM_SIZE);

    size_t res = olm_create_account(
        account->account,
        random,
        OLM_ACCOUNT_RANDOM_SIZE);

    return res != olm_error();
}

// TODO: in/outbound sessions
bool
MatrixOlmSessionTo(
    MatrixOlmSession * session,
    OlmAccount * olmAccount,
    const char * deviceId,
    const char * deviceKey,
    const char * deviceOnetimeKey)
{
    memset(session, 0, sizeof(MatrixOlmSession));

    session->deviceId = deviceId;

    session->session =
        olm_session(session->memory);

    static uint8_t random[OLM_OUTBOUND_SESSION_RANDOM_SIZE];
    Randomize(random, OLM_OUTBOUND_SESSION_RANDOM_SIZE);

    size_t res =
        olm_create_outbound_session(session->session,
            olmAccount,
            deviceKey, strlen(deviceKey),
            deviceOnetimeKey, strlen(deviceOnetimeKey),
            random, OLM_OUTBOUND_SESSION_RANDOM_SIZE);
    
    if (res == olm_error()) {
        printf("error olm: %s\n", olm_account_last_error(olmAccount));
    }

    return session->session != NULL;
}

bool
MatrixOlmSessionEncrypt(
    MatrixOlmSession * session,
    const char * plaintext,
    char * outBuffer, int outBufferCap)
{
    static uint8_t random[OLM_ENCRYPT_RANDOM_SIZE];
    Randomize(random, OLM_ENCRYPT_RANDOM_SIZE);

    size_t res = olm_encrypt(session->session,
        plaintext, strlen(plaintext),
        random, OLM_ENCRYPT_RANDOM_SIZE,
        outBuffer, outBufferCap);

    return res != olm_error();
}

// https://matrix.org/docs/guides/end-to-end-encryption-implementation-guide#starting-a-megolm-session
bool
MatrixMegolmOutSessionInit(
    MatrixMegolmOutSession * session,
    const char * roomId)
{
    memset(session, 0, sizeof(MatrixMegolmOutSession));

    static uint8_t random[MEGOLM_INIT_RANDOM_SIZE];
    Randomize(random, MEGOLM_INIT_RANDOM_SIZE);

    session->roomId = roomId;

    session->session =
        olm_outbound_group_session(session->memory);

    olm_init_outbound_group_session(
        session->session,
        random,
        MEGOLM_INIT_RANDOM_SIZE);

    olm_outbound_group_session_id(session->session,
        (uint8_t *)session->id,
        MEGOLM_SESSION_ID_SIZE);
        
    olm_outbound_group_session_key(session->session,
        (uint8_t *)session->key,
        MEGOLM_SESSION_KEY_SIZE);
    
    return true;
}

bool
MatrixMegolmOutSessionEncrypt(
    MatrixMegolmOutSession * session,
    const char * plaintext,
    char * outBuffer, int outBufferCap)
{
    size_t res = olm_group_encrypt(session->session,
        (uint8_t *)plaintext, strlen(plaintext),
        (uint8_t *)outBuffer, outBufferCap);

    return res != olm_error();
}



bool
MatrixClientInit(
    MatrixClient * client,
    const char * server)
{
    memset(client, 0, sizeof(MatrixClient));

    strcpy(client->server, server);

    // init olm account
    MatrixOlmAccountInit(&client->olmAccount);

    // set device key
    static char deviceKeysJson[OLM_IDENTITY_KEYS_JSON_SIZE];
    size_t res =
        olm_account_identity_keys(
            client->olmAccount.account,
            deviceKeysJson,
            OLM_IDENTITY_KEYS_JSON_SIZE);

    mjson_get_string(deviceKeysJson, res,
        "$.curve25519",
        client->deviceKey, DEVICE_KEY_SIZE);
    mjson_get_string(deviceKeysJson, res,
        "$.ed25519",
        client->signingKey, SIGNING_KEY_SIZE);

    return true;
}

bool
MatrixClientSetAccessToken(
    MatrixClient * client,
    const char * accessToken)
{
    int accessTokenLen = strlen(accessToken);

    if (accessTokenLen > ACCESS_TOKEN_SIZE - 1)
        return false;

    for (int i = 0; i < accessTokenLen; i++)
        client->accessToken[i] = accessToken[i];

    return true;
}

bool
MatrixClientSetDeviceId(
    MatrixClient * client,
    const char * deviceId)
{
    int deviceIdLen = strlen(deviceId);

    if (deviceIdLen > DEVICE_ID_SIZE - 1)
        return false;

    for (int i = 0; i < deviceIdLen; i++)
        client->deviceId[i] = deviceId[i];

    return true;
}

bool
MatrixClientSetUserId(
    MatrixClient * client,
    const char * userId)
{
    int userIdLen = strlen(userId);

    if (userIdLen > USER_ID_SIZE - 1)
        return false;

    for (int i = 0; i < userIdLen; i++)
        client->userId[i] = userId[i];

    return true;
}

bool
MatrixClientGenerateOnetimeKeys(
    MatrixClient * client,
    int numberOfKeys)
{
    static uint8_t random[OLM_ONETIME_KEYS_RANDOM_SIZE];
    Randomize(random, OLM_ONETIME_KEYS_RANDOM_SIZE);

    size_t res =
        olm_account_generate_one_time_keys(client->olmAccount.account,
            numberOfKeys, random, OLM_ONETIME_KEYS_RANDOM_SIZE);

    return res != olm_error();
}

// https://spec.matrix.org/v1.7/client-server-api/#post_matrixclientv3keysupload
bool
MatrixClientUploadOnetimeKeys(
    MatrixClient * client)
{
    static char requestBuffer[KEYS_UPLOAD_REQUEST_SIZE];

    mjson_snprintf(requestBuffer, KEYS_UPLOAD_REQUEST_SIZE,
        "{\"one_time_keys\":{");

    static char onetimeKeysBuffer[1024];
    olm_account_one_time_keys(client->olmAccount.account,
        onetimeKeysBuffer, 1024);

    const char *keys;
    int keysLen;
    mjson_find(onetimeKeysBuffer, strlen(onetimeKeysBuffer), "$.curve25519", &keys, &keysLen);

    int koff, klen, voff, vlen, vtype, off = 0;
    while ((off = mjson_next(keys, keysLen, off, &koff, &klen, &voff, &vlen, &vtype)) != 0) {
        static char keyJson[JSON_ONETIME_KEY_SIZE];
        
        snprintf(keyJson, JSON_ONETIME_KEY_SIZE,
            "{\"key\":\"%.*s\"}",
            vlen-2, keys + voff+1);

        static char keyJsonSigned[JSON_ONETIME_KEY_SIGNED_SIZE];

        JsonSign(client,
            keyJson, JSON_ONETIME_KEY_SIZE,
            keyJsonSigned, JSON_ONETIME_KEY_SIGNED_SIZE);
        
        mjson_snprintf(requestBuffer+strlen(requestBuffer), KEYS_UPLOAD_REQUEST_SIZE-strlen(requestBuffer),
            "\"signed_curve25519:%.*s\":%s,",
            klen-2, keys + koff+1,
            keyJsonSigned);
    }

    mjson_snprintf(requestBuffer+strlen(requestBuffer)-1, KEYS_UPLOAD_REQUEST_SIZE-strlen(requestBuffer),
        "}}");

    static char responseBuffer[KEYS_UPLOAD_RESPONSE_SIZE];
    MatrixHttpPost(client,
        KEYS_UPLOAD_URL,
        requestBuffer,
        responseBuffer, KEYS_UPLOAD_RESPONSE_SIZE,
        true);

    return true;
}

// https://spec.matrix.org/v1.7/client-server-api/#post_matrixclientv3keysupload
bool
MatrixClientUploadDeviceKeys(
    MatrixClient * client)
{
    static char deviceKeysBuffer[KEYS_UPLOAD_REQUEST_SIZE];

    mjson_snprintf(deviceKeysBuffer, KEYS_UPLOAD_REQUEST_SIZE,
        "{\"device_keys\":{"
            "\"algorithms\":[\"m.olm.v1.curve25519-aes-sha2\",\"m.megolm.v1.aes-sha2\"],"
            "\"device_id\":\"%s\","
            "\"keys\":{"
                "\"curve25519:%s\":\"%s\","
                "\"ed25519:%s\":\"%s\""
            "},"
            "\"user_id\":\"%s\""
        "}}",
        client->deviceId,
        client->deviceId, client->deviceKey,
        client->deviceId, client->signingKey,
        client->userId);

    static char deviceKeysSignedBuffer[KEYS_UPLOAD_REQUEST_SIGNED_SIZE];
    JsonSign(client,
        deviceKeysBuffer, KEYS_UPLOAD_REQUEST_SIZE,
        deviceKeysSignedBuffer, KEYS_UPLOAD_REQUEST_SIZE);


    static char responseBuffer[KEYS_UPLOAD_RESPONSE_SIZE];
    MatrixHttpPost(client,
        KEYS_UPLOAD_URL,
        deviceKeysSignedBuffer,
        responseBuffer, KEYS_UPLOAD_RESPONSE_SIZE,
        true);

    return true;
}

// https://spec.matrix.org/v1.7/client-server-api/#post_matrixclientv3keysclaim
bool
MatrixClientClaimOnetimeKey(
    MatrixClient * client,
    const char * userId,
    const char * deviceId,
    char * outOnetimeKey, int outOnetimeKeyCap)
{
    static char requestBuffer[KEYS_CLAIM_REQUEST_SIZE];
    mjson_snprintf(requestBuffer, KEYS_CLAIM_REQUEST_SIZE,
    "{"
      "\"one_time_keys\": {"
        "\"%s\": {"
          "\"%s\": \"signed_curve25519\""
        "}"
      "},"
      "\"timeout\": 10000"
    "}",
    userId,
    deviceId);

    static char responseBuffer[KEYS_CLAIM_RESPONSE_SIZE];
    MatrixHttpPost(client,
        KEYS_CLAIM_URL,
        requestBuffer,
        responseBuffer, KEYS_CLAIM_RESPONSE_SIZE,
        true);
    
    char userIdEscaped[USER_ID_SIZE];
    JsonEscape(userId, strlen(userId),
        userIdEscaped, USER_ID_SIZE);
    
    static char query[JSON_QUERY_SIZE];
    snprintf(query, JSON_QUERY_SIZE,
        "$.one_time_keys.%s.%s",
        userIdEscaped,
        deviceId);
    
    const char * keyObject;
    int keyObjectSize;
    mjson_find(responseBuffer, strlen(responseBuffer),
        query,
        &keyObject, &keyObjectSize);
    
    int koff, klen, voff, vlen, vtype;
    mjson_next(keyObject, keyObjectSize, 0,
        &koff, &klen, &voff, &vlen, &vtype);
    
    mjson_get_string(keyObject + voff, vlen,
        "$.key", outOnetimeKey, outOnetimeKeyCap);
    
    // TODO: verify signature
    
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
        client->accessToken, ACCESS_TOKEN_SIZE);
    mjson_get_string(responseBuffer, responseLen,
        "$.device_id",
        client->deviceId, DEVICE_ID_SIZE);
    mjson_get_string(responseBuffer, responseLen,
        "$.expires_in_ms",
        client->expireMs, EXPIRE_MS_SIZE);
    mjson_get_string(responseBuffer, responseLen,
        "$.refresh_token",
        client->refreshToken, REFRESH_TOKEN_SIZE);

    return true;
}

// https://spec.matrix.org/v1.6/client-server-api/#put_matrixclientv3roomsroomidsendeventtypetxnid
bool
MatrixClientSendEvent(
    MatrixClient * client,
    const char * roomId,
    const char * msgType,
    const char * msgBody)
{    
    static char requestUrl[MAX_URL_LEN];
    sprintf(requestUrl,
        ROOMEVENT_URL, roomId, msgType, (int)time(NULL));

    static char responseBuffer[ROOMEVENT_RESPONSE_SIZE];
    bool result =
        MatrixHttpPut(client,
            requestUrl,
            msgBody,
            responseBuffer, ROOMEVENT_RESPONSE_SIZE,
            true);
    
    return result;
}

// https://spec.matrix.org/v1.6/client-server-api/#mroomencrypted
// https://matrix.org/docs/guides/end-to-end-encryption-implementation-guide#sending-an-encrypted-message-event
bool
MatrixClientSendEventEncrypted(
    MatrixClient * client,
    const char * roomId,
    const char * msgType,
    const char * msgBody)
{
    // event json
    static char requestBuffer[ROOMEVENT_REQUEST_SIZE];
    sprintf(requestBuffer,
        "{"
        "\"type\":\"%s\","
        "\"content\":%s,"
        "\"room_id\":\"%s\""
        "}",
        msgType,
        msgBody,
        roomId);

    // get megolm session
    MatrixMegolmOutSession * outSession;
    MatrixClientGetMegolmOutSession(client, roomId, &outSession);
        
    // encrypt
    static char encryptedBuffer[ENCRYPTED_REQUEST_SIZE];
    MatrixMegolmOutSessionEncrypt(outSession,
        requestBuffer,
        encryptedBuffer, ENCRYPTED_REQUEST_SIZE);

    // encrypted event json
    const char * senderKey = client->deviceKey;
    const char * sessionId = outSession->id;
    const char * deviceId = client->deviceId;

    static char encryptedEventBuffer[ENCRYPTED_EVENT_SIZE];
    sprintf(encryptedEventBuffer,
        "{"
        "\"algorithm\":\"m.megolm.v1.aes-sha2\","
        "\"ciphertext\":\"%s\","
        "\"device_id\":\"%s\","
        "\"sender_key\":\"%s\","
        "\"session_id\":\"%s\""
        "}",
        encryptedBuffer,
        deviceId,
        senderKey,
        sessionId);

    // send
    return MatrixClientSendEvent(client,
        roomId,
        "m.room.encrypted",
        encryptedEventBuffer);
}

// https://spec.matrix.org/v1.6/client-server-api/#get_matrixclientv3sync
bool
MatrixClientSync(
    MatrixClient * client,
    char * outSyncBuffer, int outSyncCap)
{
    return
        MatrixHttpGet(client,
            "/_matrix/client/v3/sync",
            outSyncBuffer, outSyncCap,
            true);
}

bool
MatrixClientShareMegolmOutSession(
    MatrixClient * client,
    const char * userId,
    const char * deviceId,
    MatrixMegolmOutSession * session)
{
    // generate room key event
    static char eventBuffer[KEY_SHARE_EVENT_LEN];
    sprintf(eventBuffer,
        "{"
            "\"algorithm\":\"m.megolm.v1.aes-sha2\","
            "\"room_id\":\"%s\","
            "\"session_id\":\"%s\","
            "\"session_key\":\"%s\""
        "}",
        session->roomId,
        session->id,
        session->key
    );

    // // get olm session
    // MatrixOlmSession * olmSession;
    // MatrixClientGetOlmSession(client, userId, deviceId, &olmSession);

    // // encrypt
    // char encryptedBuffer[KEY_SHARE_EVENT_LEN];
    // MatrixOlmSessionEncrypt(olmSession,
    //     eventBuffer,
    //     encryptedBuffer, KEY_SHARE_EVENT_LEN);

    // send
    MatrixClientSendToDeviceEncrypted(client,
        userId,
        deviceId,
        eventBuffer,
        "m.room_key");

    return true;
}

bool
MatrixClientShareMegolmOutSessionTest(
    MatrixClient * client,
    const char * deviceId,
    MatrixMegolmOutSession * session)
{
    // generate room key event
    char eventBuffer[KEY_SHARE_EVENT_LEN];
    sprintf(eventBuffer,
        "{"
            "\"algorithm\":\"m.megolm.v1.aes-sha2\","
            "\"room_id\":\"%s\","
            "\"session_id\":\"%s\","
            "\"session_key\":\"%s\""
        "}",
        session->roomId,
        session->id,
        session->key
    );

    // send
    MatrixClientSendToDevice(client,
        client->userId,
        deviceId,
        eventBuffer,
        "m.room_key");

    return true;
}

// bool
// MatrixClientSetMegolmOutSession(
//     MatrixClient * client,
//     const char * roomId,
//     MatrixMegolmOutSession session)
// {
//     if (client->numMegolmOutSessions < 10)
//     {
//         session.roomId = roomId;
//         client->megolmOutSessions[client->numMegolmOutSessions] = session;
//         client->numMegolmOutSessions++;

//         return true;
//     }
//     return false;
// }

bool
MatrixClientGetMegolmOutSession(
    MatrixClient * client,
    const char * roomId,
    MatrixMegolmOutSession ** outSession)
{
    for (int i = 0; i < client->numMegolmOutSessions; i++)
    {
        if (strcmp(client->megolmOutSessions[i].roomId, roomId) == 0)
        {
            *outSession = &client->megolmOutSessions[i];
            return true;
        }
    }

    if (client->numMegolmOutSessions < NUM_MEGOLM_SESSIONS)
    {
        MatrixMegolmOutSessionInit(
            &client->megolmOutSessions[client->numMegolmOutSessions],
            roomId);

        *outSession = &client->megolmOutSessions[client->numMegolmOutSessions];
        
        client->numMegolmOutSessions++;

        return true;
    }

    return false;
}

bool
MatrixClientGetOlmSession(
    MatrixClient * client,
    const char * userId,
    const char * deviceId,
    MatrixOlmSession ** outSession)
{
    for (int i = 0; i < client->numOlmSessions; i++)
    {
        if (strcmp(client->olmSessions[i].deviceId, deviceId) == 0)
        {
            *outSession = &client->olmSessions[i];
            return true;
        }
    }

    if (client->numOlmSessions < NUM_OLM_SESSIONS)
    {
        static char deviceKey[DEVICE_KEY_SIZE];
        MatrixClientGetDeviceKey(client,
            deviceId,
            deviceKey, DEVICE_KEY_SIZE);

        char onetimeKey[ONETIME_KEY_SIZE];
        MatrixClientClaimOnetimeKey(client,
            userId,
            deviceId,
            onetimeKey, ONETIME_KEY_SIZE);

        MatrixOlmSessionTo(
            &client->olmSessions[client->numOlmSessions],
            client->olmAccount.account,
            deviceId,
            deviceKey,
            onetimeKey);

        *outSession = &client->olmSessions[client->numOlmSessions];
        
        client->numOlmSessions++;

        return true;
    }

    return false;
}

// https://spec.matrix.org/v1.6/client-server-api/#put_matrixclientv3sendtodeviceeventtypetxnid
bool
MatrixClientSendToDevice(
    MatrixClient * client,
    const char * userId,
    const char * deviceId,
    const char * message,
    const char * msgType)
{
    static char requestUrl[MAX_URL_LEN];
    sprintf(requestUrl,
        TODEVICE_URL, msgType, (int)time(NULL));

    static char eventBuffer[TODEVICE_EVENT_SIZE];
    snprintf(eventBuffer, TODEVICE_EVENT_SIZE,
        "{"
            "\"messages\": {"
                "\"%s\": {"
                    "\"%s\":%s"
                "}"
            "}"
        "}",
        userId,
        deviceId,
        message);

    static char responseBuffer[ROOMEVENT_RESPONSE_SIZE];
    bool result =
        MatrixHttpPut(client,
            requestUrl,
            eventBuffer,
            responseBuffer, ROOMEVENT_RESPONSE_SIZE,
            true);
    
    return result;
}

bool
MatrixClientSendToDeviceEncrypted(
    MatrixClient * client,
    const char * userId,
    const char * deviceId,
    const char * message,
    const char * msgType)
{
    // get olm session
    MatrixOlmSession * olmSession;
    MatrixClientGetOlmSession(client, userId, deviceId, &olmSession);

    // create event json
    char deviceKey[DEVICE_KEY_SIZE];
    MatrixClientGetDeviceKey(client, deviceId, deviceKey, DEVICE_KEY_SIZE);
    
    static char eventBuffer[TODEVICE_EVENT_SIZE];
    sprintf(eventBuffer,
        "{"
        "\"type\": \"%s\","
        "\"content\": \"%s\","
        "\"sender\": \"%s\","
        "\"recipient\": \"%s\","
        "\"recipient_keys\": {"
          "\"ed25519\": \"%s\""
        "},"
        "\"keys\": {"
          "\"ed25519\": \"%s\""
        "}"
        "}",
        msgType,
        message,
        client->userId,
        userId, // recipient user id
        deviceKey, // recipient device key
        client->deviceKey);

    // encrypt
    static char encryptedBuffer[ENCRYPTED_REQUEST_SIZE];
    MatrixOlmSessionEncrypt(olmSession,
        eventBuffer,
        encryptedBuffer, ENCRYPTED_REQUEST_SIZE);

    static char encryptedEventBuffer[ENCRYPTED_EVENT_SIZE];
    sprintf(encryptedEventBuffer,
        "{"
        "\"algorithm\":\"m.olm.v1.curve25519-aes-sha2\","
        "\"ciphertext\":{"
          "\"%s\":{"
            "\"body\":\"%s\","
            "\"type\":%d"
          "}"
        "},"
        "\"device_id\":\"%s\","
        "\"sender_key\":\"%s\""
        "}",
        deviceKey,
        encryptedBuffer,
        0, //olmSession->type,
        client->deviceId,
        client->deviceKey);

    // send
    return MatrixClientSendToDevice(
        client,
        userId,
        deviceId,
        encryptedEventBuffer,
        "m.room.encrypted");
}

bool
MatrixClientFindDevice(
    MatrixClient * client,
    const char * deviceId,
    MatrixDevice ** outDevice)
{
    MatrixClientRequestDeviceKeys(client);

    for (int i = 0; i < client->numDevices; i++)
    {
        if (strcmp(client->devices[i].deviceId, deviceId) == 0)
        {
            *outDevice = &client->devices[i];
            return true;
        }
    }

    *outDevice = NULL;
    return false;
}

bool
MatrixClientGetDeviceKey(
    MatrixClient * client,
    const char * deviceId,
    char * outDeviceKey, int outDeviceKeyCap)
{
    MatrixDevice * device;
    
    if (MatrixClientFindDevice(client, deviceId, &device))
    {
        strncpy(outDeviceKey, device->deviceKey, outDeviceKeyCap);
        return true;
    }

    MatrixClientRequestDeviceKeys(client);
    
    if (MatrixClientFindDevice(client, deviceId, &device))
    {
        strncpy(outDeviceKey, device->deviceKey, outDeviceKeyCap);
        return true;
    }

    return false;
}

// https://spec.matrix.org/v1.6/client-server-api/#post_matrixclientv3keysquery
bool
MatrixClientRequestDeviceKeys(
    MatrixClient * client)
{
    static char userIdEscaped[USER_ID_SIZE];
    JsonEscape(client->userId, strlen(client->userId),
        userIdEscaped, USER_ID_SIZE);

    static char request[KEYS_QUERY_REQUEST_SIZE];
    snprintf(request, KEYS_QUERY_REQUEST_SIZE,
        "{\"device_keys\":{\"%s\":[]}}", client->userId);

    static char responseBuffer[KEYS_QUERY_RESPONSE_SIZE];
    bool requestResult = MatrixHttpPost(client,
        KEYS_QUERY_URL,
        request,
        responseBuffer, KEYS_QUERY_RESPONSE_SIZE,
        true);

    if (! requestResult)
        return false;

    // query for retrieving device keys for user id
    static char query[JSON_QUERY_SIZE];
    snprintf(query, JSON_QUERY_SIZE,
        "$.device_keys.%s", userIdEscaped);
    
    const char * s;
    int slen;
    mjson_find(responseBuffer, strlen(responseBuffer),
        query, &s, &slen);

    // loop over keys
    
    int koff, klen, voff, vlen, vtype, off = 0;
    for (off = 0; (off = mjson_next(s, slen, off, &koff, &klen,
                                    &voff, &vlen, &vtype)) != 0; ) {
        const char * key = s + koff;
        const char * val = s + voff;

        // set device id, "key" is the JSON key
        MatrixDevice d;
        snprintf(d.deviceId, DEVICE_ID_SIZE,
            "%.*s", klen-2, key+1);

        // look for device key in value
        static char deviceKeyQuery[JSON_QUERY_SIZE];
        snprintf(deviceKeyQuery, JSON_QUERY_SIZE,
            "$.keys.curve25519:%s", d.deviceId);
        mjson_get_string(val, vlen,
            deviceKeyQuery, d.deviceKey, DEVICE_KEY_SIZE);

        // add device
        if (client->numDevices < NUM_DEVICES)
        {
            client->devices[client->numDevices] = d;
            client->numDevices++;
        }
        else
        {
            return false;
        }
    }

    return true;
}
