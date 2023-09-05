#include "matrix.h"

#include <time.h>
#include <stdio.h>
#include <mjson.h>


#define LOGIN_REQUEST_SIZE 1024
#define LOGIN_RESPONSE_SIZE 1024
#define LOGIN_URL "/_matrix/client/v3/login"

#define ENCRYPTED_REQUEST_SIZE (1024*5)
#define ENCRYPTED_EVENT_SIZE (1024*10)
#define ROOM_SEND_REQUEST_SIZE 256
#define ROOM_SEND_RESPONSE_SIZE 1024
#define ROOM_SEND_URL "/_matrix/client/v3/rooms/%s/send/%s/%d"

#define ROOMKEY_REQUEST_SIZE (1024*4)

#define TODEVICE_EVENT_SIZE (1024*5)
#define TODEVICE_URL "/_matrix/client/v3/sendToDevice/%s/%d"

#define KEYS_QUERY_URL "/_matrix/client/v3/keys/query"
#define KEYS_QUERY_REQUEST_SIZE 256
#define KEYS_QUERY_RESPONSE_SIZE (1024*10)

#define KEYS_UPLOAD_URL "/_matrix/client/v3/keys/upload"
#define KEYS_UPLOAD_REQUEST_SIZE 1024*4
#define KEYS_UPLOAD_REQUEST_SIGNED_SIZE 2048*4
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
    
    static char thisSigningKey[SIGNING_KEY_SIZE];
    MatrixOlmAccountGetSigningKey(&client->olmAccount, thisSigningKey, SIGNING_KEY_SIZE);

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
            //"1",
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

bool
MatrixOlmAccountUnpickle(
    MatrixOlmAccount * account,
    void * pickled, int pickledLen,
    const void * key, int keyLen)
{
    size_t res;
    res = olm_unpickle_account(account->account,
        key, keyLen,
        pickled, pickledLen);
    if (res == olm_error()) {
        printf("error unpickling olm account:%s\n",
            olm_account_last_error(account->account));
    }
    return res != olm_error();
}

bool
MatrixOlmAccountGetDeviceKey(
    MatrixOlmAccount * account,
    char * key, int keyCap)
{
    static char deviceKeysJson[OLM_IDENTITY_KEYS_JSON_SIZE];
    size_t res =
        olm_account_identity_keys(account->account,
            deviceKeysJson, OLM_IDENTITY_KEYS_JSON_SIZE);
    mjson_get_string(deviceKeysJson, res,
        "$.curve25519",
        key, keyCap);
    return true;
}

bool
MatrixOlmAccountGetSigningKey(
    MatrixOlmAccount * account,
    char * key, int keyCap)
{
    static char deviceKeysJson[OLM_IDENTITY_KEYS_JSON_SIZE];
    size_t res =
        olm_account_identity_keys(account->account,
            deviceKeysJson, OLM_IDENTITY_KEYS_JSON_SIZE);
    mjson_get_string(deviceKeysJson, res,
        "$.ed25519",
        key, keyCap);
    return true;
}

// TODO:in/outbound sessions
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
        printf("error olm:%s\n", olm_session_last_error(session->session));
    }

    return session->session != NULL;
}

bool
MatrixOlmSessionUnpickle(
    MatrixOlmSession * session,
    const char * deviceId,
    void * pickled, int pickledLen,
    const void * key, int keyLen)
{
    memset(session, 0, sizeof(MatrixOlmSession));

    session->deviceId = deviceId;

    session->session =
        olm_session(session->memory);
    
    size_t res;
    res = olm_unpickle_session(session->session,
        key, keyLen,
        pickled, pickledLen);
    
    if (res == olm_error()) {
        printf("error unpickling olm session:%s\n", olm_session_last_error(session->session));
    }

    return res != olm_error();
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

bool
MatrixOlmSessionDecrypt(
    MatrixOlmSession * session,
    size_t messageType,
    char * encrypted,
    char * outBuffer, int outBufferCap)
{
    static uint8_t random[OLM_ENCRYPT_RANDOM_SIZE];
    Randomize(random, OLM_ENCRYPT_RANDOM_SIZE);

    size_t res =
        olm_decrypt(session->session,
            messageType,
            encrypted, strlen(encrypted),
            outBuffer, outBufferCap);
    
    if (res != olm_error() && res < outBufferCap)
        outBuffer[outBufferCap] = '\0';

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

    strncpy(session->roomId, roomId, ROOM_ID_SIZE);

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
MatrixMegolmOutSessionSave(
    MatrixMegolmOutSession * session,
    const char * filename,
    const char * key)
{
    FILE * f = fopen(filename, "w");

    size_t roomIdLen = strlen(session->roomId);
    fwrite(&roomIdLen, sizeof(size_t), 1, f);
    fwrite(session->roomId, 1, roomIdLen, f);

    size_t pickleBufferLen =
        olm_pickle_outbound_group_session_length(
            session->session);
    void * pickleBuffer = malloc(pickleBufferLen);

    olm_pickle_outbound_group_session(
        session->session,
        key, strlen(key),
        pickleBuffer, pickleBufferLen);
    
    fwrite(&pickleBufferLen, sizeof(size_t), 1, f);
    fwrite(pickleBuffer, 1, pickleBufferLen, f);
    free(pickleBuffer);

    fclose(f);

    return true;
}

bool
MatrixMegolmOutSessionLoad(
    MatrixMegolmOutSession * session,
    const char * filename,
    const char * key)
{
    FILE * f = fopen(filename, "r");

    size_t roomIdLen;
    fread(&roomIdLen, sizeof(size_t), 1, f);
    fread(session->roomId, 1, roomIdLen, f);
    for (int i = roomIdLen; i < ROOM_ID_SIZE; i++)
        session->roomId[i] = '\0';

    size_t pickleBufferLen;
    fread(&pickleBufferLen, sizeof(size_t), 1, f);

    void * pickleBuffer = malloc(pickleBufferLen);
    fread(pickleBuffer, 1, pickleBufferLen, f);

    olm_unpickle_outbound_group_session(
        session->session,
        key, strlen(key),
        pickleBuffer, pickleBufferLen);
    
    free(pickleBuffer);

    olm_outbound_group_session_id(session->session, (uint8_t *)session->id, MEGOLM_SESSION_ID_SIZE);
    olm_outbound_group_session_key(session->session, (uint8_t *)session->key, MEGOLM_SESSION_KEY_SIZE);

    fclose(f);

    return true;
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

    return true;
}

bool
MatrixClientSave(
    MatrixClient * client,
    const char * filename)
{
    FILE * f = fopen(filename, "w");
    
    
    char thisDeviceKey[DEVICE_KEY_SIZE];
    MatrixOlmAccountGetDeviceKey(&client->olmAccount, thisDeviceKey, DEVICE_KEY_SIZE);
    char thisSigningKey[DEVICE_KEY_SIZE];
    MatrixOlmAccountGetSigningKey(&client->olmAccount, thisSigningKey, DEVICE_KEY_SIZE);


    fwrite(thisDeviceKey, 1, DEVICE_KEY_SIZE, f);
    fwrite(thisSigningKey, 1, DEVICE_KEY_SIZE, f);
    fwrite(client->userId, 1, USER_ID_SIZE, f);
    fwrite(client->server, 1, SERVER_SIZE, f);
    fwrite(client->accessToken, 1, ACCESS_TOKEN_SIZE, f);
    fwrite(client->deviceId, 1, DEVICE_ID_SIZE, f);
    fwrite(client->expireMs, 1, EXPIRE_MS_SIZE, f);
    fwrite(client->refreshToken, 1, REFRESH_TOKEN_SIZE, f);

    fwrite(&client->numDevices, sizeof(int), 1, f);
    for (int i = 0; i < client->numDevices; i++) {
        fwrite(client->devices[i].deviceId, 1, DEVICE_ID_SIZE, f);
        fwrite(client->devices[i].deviceKey, 1, DEVICE_KEY_SIZE, f);
    }

    fclose(f);
    return true;
}

bool
MatrixClientLoad(
    MatrixClient * client,
    const char * filename)
{
    FILE * f = fopen(filename, "r");
    
    
    char thisDeviceKey[DEVICE_KEY_SIZE];
    MatrixOlmAccountGetDeviceKey(&client->olmAccount, thisDeviceKey, DEVICE_KEY_SIZE);
    char thisSigningKey[DEVICE_KEY_SIZE];
    MatrixOlmAccountGetSigningKey(&client->olmAccount, thisSigningKey, DEVICE_KEY_SIZE);


    fread(thisDeviceKey, 1, DEVICE_KEY_SIZE, f);
    fread(thisSigningKey, 1, DEVICE_KEY_SIZE, f);
    fread(client->userId, 1, USER_ID_SIZE, f);
    fread(client->server, 1, SERVER_SIZE, f);
    fread(client->accessToken, 1, ACCESS_TOKEN_SIZE, f);
    fread(client->deviceId, 1, DEVICE_ID_SIZE, f);
    fread(client->expireMs, 1, EXPIRE_MS_SIZE, f);
    fread(client->refreshToken, 1, REFRESH_TOKEN_SIZE, f);

    fread(&client->numDevices, sizeof(int), 1, f);
    for (int i = 0; i < client->numDevices; i++) {
        fread(client->devices[i].deviceId, 1, DEVICE_ID_SIZE, f);
        fread(client->devices[i].deviceKey, 1, DEVICE_KEY_SIZE, f);
    }

    fclose(f);
    return true;
}

bool
MatrixClientSetAccessToken(
    MatrixClient * client,
    const char * accessToken)
{
    for (int i = 0; i < ACCESS_TOKEN_SIZE-1; i++)
        client->accessToken[i] = accessToken[i];
    client->accessToken[ACCESS_TOKEN_SIZE-1] = '\0';

    return true;
}

bool
MatrixClientSetDeviceId(
    MatrixClient * client,
    const char * deviceId)
{
    for (int i = 0; i < DEVICE_ID_SIZE-1; i++)
        client->deviceId[i] = deviceId[i];
    client->deviceId[DEVICE_ID_SIZE-1] = '\0';

    return true;
}

bool
MatrixClientSetUserId(
    MatrixClient * client,
    const char * userId)
{
    for (int i = 0; i < USER_ID_SIZE-1; i++)
        client->userId[i] = userId[i];
    client->userId[USER_ID_SIZE-1] = '\0';

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
        "{");

    static char onetimeKeysBuffer[1024];
    olm_account_one_time_keys(client->olmAccount.account,
        onetimeKeysBuffer, 1024);

    const char *keys;
    int keysLen;
    mjson_find(onetimeKeysBuffer, strlen(onetimeKeysBuffer), "$.curve25519", &keys, &keysLen);

    int koff, klen, voff, vlen, vtype, off = 0;
    while ((off = mjson_next(keys, keysLen, off, &koff, &klen, &voff, &vlen, &vtype)) != 0) {
        static char keyJson[JSON_ONETIME_KEY_SIZE];
        
        int keyJsonLen =
            snprintf(keyJson, JSON_ONETIME_KEY_SIZE,
                "{\"key\":\"%.*s\"}",
                vlen-2, keys + voff+1);

        static char keyJsonSigned[JSON_ONETIME_KEY_SIGNED_SIZE];

        JsonSign(client,
            keyJson, keyJsonLen,
            keyJsonSigned, JSON_ONETIME_KEY_SIGNED_SIZE);
        
        mjson_snprintf(requestBuffer+strlen(requestBuffer), KEYS_UPLOAD_REQUEST_SIZE-strlen(requestBuffer),
            "\"signed_curve25519:%.*s\":%s,",
            klen-2, keys + koff+1,
            keyJsonSigned);
    }

    if (requestBuffer[strlen(requestBuffer)-1] == ',')
        requestBuffer[strlen(requestBuffer)-1] = '\0';

    mjson_snprintf(requestBuffer+strlen(requestBuffer), KEYS_UPLOAD_REQUEST_SIZE-strlen(requestBuffer),
        "}");
        
    // static char onetimeKeysSignedBuffer[KEYS_UPLOAD_REQUEST_SIGNED_SIZE];
    // JsonSign(client,
    //     requestBuffer, strlen(requestBuffer),
    //     onetimeKeysSignedBuffer, KEYS_UPLOAD_REQUEST_SIZE);
        
    // static char finalEvent[KEYS_UPLOAD_REQUEST_SIGNED_SIZE];
    // snprintf(finalEvent, KEYS_UPLOAD_REQUEST_SIGNED_SIZE,
    // "{\"one_time_keys\":%s}", onetimeKeysSignedBuffer);
    static char finalEvent[KEYS_UPLOAD_REQUEST_SIGNED_SIZE];
    snprintf(finalEvent, KEYS_UPLOAD_REQUEST_SIGNED_SIZE,
    "{\"one_time_keys\":%s}", requestBuffer);

    static char responseBuffer[KEYS_UPLOAD_RESPONSE_SIZE];
    MatrixHttpPost(client,
        KEYS_UPLOAD_URL,
        finalEvent,
        responseBuffer, KEYS_UPLOAD_RESPONSE_SIZE,
        true);

    printf("%s\n", responseBuffer);

    return true;
}

// https://spec.matrix.org/v1.7/client-server-api/#post_matrixclientv3keysupload
bool
MatrixClientUploadDeviceKey(
    MatrixClient * client)
{
    char thisDeviceKey[DEVICE_KEY_SIZE];
    MatrixOlmAccountGetDeviceKey(&client->olmAccount, thisDeviceKey, DEVICE_KEY_SIZE);
    char thisSigningKey[DEVICE_KEY_SIZE];
    MatrixOlmAccountGetSigningKey(&client->olmAccount, thisSigningKey, DEVICE_KEY_SIZE);

    static char deviceKeysBuffer[KEYS_UPLOAD_REQUEST_SIZE];

    int deviceKeysBufferLen =
        mjson_snprintf(deviceKeysBuffer, KEYS_UPLOAD_REQUEST_SIZE,
            "{"
                "\"algorithms\":[\"m.olm.v1.curve25519-aes-sha2\",\"m.megolm.v1.aes-sha2\"],"
                "\"device_id\":\"%s\","
                "\"keys\":{"
                    "\"curve25519:%s\":\"%s\","
                    "\"ed25519:%s\":\"%s\""
                "},"
                "\"user_id\":\"%s\""
            "}",
            client->deviceId,
            client->deviceId, thisDeviceKey,
            client->deviceId, thisSigningKey,
            client->userId);

    static char deviceKeysSignedBuffer[KEYS_UPLOAD_REQUEST_SIGNED_SIZE];
    JsonSign(client,
        deviceKeysBuffer, deviceKeysBufferLen,
        deviceKeysSignedBuffer, KEYS_UPLOAD_REQUEST_SIZE);
    
    static char finalEvent[KEYS_UPLOAD_REQUEST_SIGNED_SIZE];
    snprintf(finalEvent, KEYS_UPLOAD_REQUEST_SIGNED_SIZE,
    "{\"device_keys\":%s}", deviceKeysSignedBuffer);

    static char responseBuffer[KEYS_UPLOAD_RESPONSE_SIZE];
    MatrixHttpPost(client,
        KEYS_UPLOAD_URL,
        finalEvent,
        responseBuffer, KEYS_UPLOAD_RESPONSE_SIZE,
        true);
        
    printf("%s\n", responseBuffer);

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
      "\"one_time_keys\":{"
        "\"%s\":{"
          "\"%s\":\"signed_curve25519\""
        "}"
      "},"
      "\"timeout\":10000"
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
    
    // TODO:verify signature
    
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
            "\"type\":\"m.login.password\","
            "\"identifier\":{"
                "\"type\":\"m.id.user\","
                "\"user\":\"%s\""
            "},"
            "\"password\":\"%s\","
            "\"initial_device_display_name\":\"%s\""
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
    
    if (!result)
        return false;
    
    int responseLen = strlen(responseBuffer);

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
        ROOM_SEND_URL, roomId, msgType, (int)time(NULL));

    static char responseBuffer[ROOM_SEND_RESPONSE_SIZE];
    bool result =
        MatrixHttpPut(client,
            requestUrl,
            msgBody,
            responseBuffer, ROOM_SEND_RESPONSE_SIZE,
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
    static char requestBuffer[ROOM_SEND_REQUEST_SIZE];
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

    char thisDeviceKey[DEVICE_KEY_SIZE];
    MatrixOlmAccountGetDeviceKey(&client->olmAccount, thisDeviceKey, DEVICE_KEY_SIZE);
    

    // encrypted event json
    const char * senderKey = thisDeviceKey;
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

// https://spec.matrix.org/v1.7/client-server-api/#get_matrixclientv3roomsroomideventeventid
bool
MatrixClientGetRoomEvent(
    MatrixClient * client,
    const char * roomId,
    const char * eventId,
    char * outEvent, int outEventCap)
{
    static char url[MAX_URL_LEN];
    snprintf(url, MAX_URL_LEN,
        "/_matrix/client/v3/rooms/%s/event/%s",
            roomId,
            eventId);

    return
        MatrixHttpGet(client,
            url,
            outEvent, outEventCap,
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
    const char * userId,
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
        userId,
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

    if (MatrixClientInitMegolmOutSession(client, roomId)) {
        *outSession = &client->megolmOutSessions[client->numMegolmOutSessions-1];
        return true;
    }

    return false;
}

bool
MatrixClientInitMegolmOutSession(
    MatrixClient * client,
    const char * roomId)
{
    if (client->numMegolmOutSessions < NUM_MEGOLM_SESSIONS)
    {
        MatrixMegolmOutSessionInit(
            &client->megolmOutSessions[client->numMegolmOutSessions],
            roomId);
        
        client->numMegolmOutSessions++;

        return true;
    }
    return false;
}

bool
MatrixClientRequestMegolmInSession(
    MatrixClient * client,
    const char * roomId,
    const char * sessionId,
    const char * senderKey,
    const char * userId,
    const char * deviceId,
    MatrixMegolmInSession * outMegolmInSession)
{
    // TODO: cancel requests
    MatrixClientSendDummy(client, userId, deviceId);

    static char event[ROOMKEY_REQUEST_SIZE];
    snprintf(event, ROOMKEY_REQUEST_SIZE,
        "{"
            "\"action\":\"request\","
            "\"body\":{"
                "\"algorithm\":\"m.megolm.v1.aes-sha2\","
                "\"room_id\":\"%s\","
                "\"sender_key\":\"%s\","
                "\"session_id\":\"%s\""
            "},"
            "\"request_id\":\"%d\","
            "\"requesting_device_id\":\"%s\""
        "}",
        roomId,
        senderKey,
        sessionId,
        time(NULL),
        client->deviceId);

    
    MatrixClientSendToDevice(client,
        userId,
        deviceId,
        event,
        "m.room_key_request");

    return true;
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
        MatrixClientRequestDeviceKey(client,
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
            "\"messages\":{"
                "\"%s\":{"
                    "\"%s\":%s"
                "}"
            "}"
        "}",
        userId,
        deviceId,
        message);

    static char responseBuffer[ROOM_SEND_RESPONSE_SIZE];
    bool result =
        MatrixHttpPut(client,
            requestUrl,
            eventBuffer,
            responseBuffer, ROOM_SEND_RESPONSE_SIZE,
            true);
    
    printf("%s\n", responseBuffer);
    
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
    char targetDeviceKey[DEVICE_KEY_SIZE];
    MatrixClientRequestDeviceKey(client, deviceId, targetDeviceKey, DEVICE_KEY_SIZE);
    char targetSigningKey[SIGNING_KEY_SIZE];
    MatrixClientRequestSigningKey(client, deviceId, targetSigningKey, SIGNING_KEY_SIZE);
    
    char thisSigningKey[DEVICE_KEY_SIZE];
    MatrixOlmAccountGetSigningKey(&client->olmAccount, thisSigningKey, DEVICE_KEY_SIZE);

    static char eventBuffer[TODEVICE_EVENT_SIZE];
    sprintf(eventBuffer,
        "{"
        "\"type\":\"%s\","
        "\"content\":%s,"
        "\"sender\":\"%s\","
        "\"recipient\":\"%s\","
        "\"recipient_keys\":{"
          "\"ed25519\":\"%s\""
        "},"
        "\"keys\":{"
          "\"ed25519\":\"%s\""
        "}"
        "}",
        msgType,
        message,
        client->userId,
        userId, // recipient user id
        targetSigningKey, // recipient device key
        thisSigningKey);
    
    printf("%s\n", eventBuffer);

    // encrypt
    static char encryptedBuffer[ENCRYPTED_REQUEST_SIZE];
    MatrixOlmSessionEncrypt(olmSession,
        eventBuffer,
        encryptedBuffer, ENCRYPTED_REQUEST_SIZE);

    char thisDeviceKey[DEVICE_KEY_SIZE];
    MatrixOlmAccountGetDeviceKey(&client->olmAccount, thisDeviceKey, DEVICE_KEY_SIZE);


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
        targetDeviceKey,
        encryptedBuffer,
        olm_session_has_received_message(olmSession->session),
        client->deviceId,
        thisDeviceKey);

    // send
    return MatrixClientSendToDevice(
        client,
        userId,
        deviceId,
        encryptedEventBuffer,
        "m.room.encrypted");
}

bool
MatrixClientSendDummy(
    MatrixClient * client,
    const char * userId,
    const char * deviceId)
{
    return MatrixClientSendToDeviceEncrypted(
        client,
        userId,
        deviceId,
        "{}",
        "m.dummy");
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
MatrixClientRequestDeviceKey(
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

bool
MatrixClientRequestSigningKey(
    MatrixClient * client,
    const char * deviceId,
    char * outSigningKey, int outSigningKeyCap)
{
    MatrixDevice * device;
    
    if (MatrixClientFindDevice(client, deviceId, &device))
    {
        strncpy(outSigningKey, device->signingKey, outSigningKeyCap);
        return true;
    }

    MatrixClientRequestDeviceKeys(client);
    
    if (MatrixClientFindDevice(client, deviceId, &device))
    {
        strncpy(outSigningKey, device->signingKey, outSigningKeyCap);
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

    printf("keys:\n%s\n", responseBuffer);

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

        // look for signing key in value
        static char signingKeyQuery[JSON_QUERY_SIZE];
        snprintf(signingKeyQuery, JSON_QUERY_SIZE,
            "$.keys.ed25519:%s", d.deviceId);
        mjson_get_string(val, vlen,
            signingKeyQuery, d.signingKey, SIGNING_KEY_SIZE);

        // add device
        if (client->numDevices < NUM_DEVICES)
        {
            bool foundDevice = false;
            for (int i = 0; i < client->numDevices; i++)
                if (strcmp(client->devices[i].deviceId, d.deviceId) == 0)
                    foundDevice = true;

            if (! foundDevice) {
                printf("new device: %s %s %s\n", d.deviceId, d.deviceKey, d.signingKey);
                client->devices[client->numDevices] = d;
                client->numDevices++;
            }
        }
        else
        {
            return false;
        }
    }

    return true;
}

bool
MatrixClientDeleteDevice(
    MatrixClient * client)
{
    static char deleteRequest[1024];
    snprintf(deleteRequest, 1024,
        "{\"devices\":[\"%s\"]}",
        client->deviceId);
    static char deleteResponse[1024];
    bool res = MatrixHttpPost(client, "/_matrix/client/v3/delete_devices",
        deleteRequest, deleteResponse, 1024, true);
    return res;
}