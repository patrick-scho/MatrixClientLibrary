#include "matrix.h"

#include <time.h>
#include <stdio.h>
#include <mjson.h>

#ifdef ESP_PLATFORM
#include <esp_random.h>
#endif

#define STATIC static

#define LOGIN_REQUEST_SIZE 1024
#define LOGIN_RESPONSE_SIZE 1024
#define LOGIN_URL "/_matrix/client/v3/login"

#define ENCRYPTED_REQUEST_SIZE (1024*5)
STATIC char g_EncryptedRequestBuffer[ENCRYPTED_REQUEST_SIZE];
#define ENCRYPTED_EVENT_SIZE (1024*10)
STATIC char g_EncryptedEventBuffer[ENCRYPTED_EVENT_SIZE];
#define ROOM_SEND_REQUEST_SIZE 256
#define ROOM_SEND_RESPONSE_SIZE 1024
#define ROOM_SEND_URL "/_matrix/client/v3/rooms/%s/send/%s/%d"

#define ROOMKEY_REQUEST_SIZE (1024*4)

#define TODEVICE_EVENT_SIZE (1024*5)
STATIC char g_TodeviceEventBuffer[TODEVICE_EVENT_SIZE];
#define TODEVICE_URL "/_matrix/client/v3/sendToDevice/%s/%d"

#define KEYS_QUERY_URL "/_matrix/client/v3/keys/query"
#define KEYS_QUERY_REQUEST_SIZE 256
#define KEYS_QUERY_RESPONSE_SIZE (1024*5)

#define KEYS_UPLOAD_URL "/_matrix/client/v3/keys/upload"
#define KEYS_UPLOAD_REQUEST_SIZE 1024*4
STATIC char g_KeysUploadRequestBuffer[KEYS_UPLOAD_REQUEST_SIZE];
#define KEYS_UPLOAD_REQUEST_SIGNED_SIZE 2048*4
STATIC char g_KeysUploadRequestSignedBuffer[KEYS_UPLOAD_REQUEST_SIGNED_SIZE];
#define KEYS_UPLOAD_RESPONSE_SIZE 2048

#define KEYS_CLAIM_URL "/_matrix/client/v3/keys/claim"
#define KEYS_CLAIM_REQUEST_SIZE 1024
#define KEYS_CLAIM_RESPONSE_SIZE 1024

#define SYNC_TIMEOUT 5000

#define JSON_QUERY_SIZE 128
#define JSON_MAX_INDICES 100
#define JSON_MAX_ENTRY_SIZE 1024

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))

void
Randomize(
    uint8_t * random,
    int randomLen)
{
    #ifdef ESP_PLATFORM

    for (int i = 0; i < randomLen; i++)
    {
        random[i] = esp_random() % 256;
    }

    #else

    STATIC bool first = true;
    if (first) { srand(time(0)); first = false; }

    for (int i = 0; i < randomLen; i++)
    {
        random[i] = rand() % 256;
    }

    #endif
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

bool
JsonCanonicalize(
    const char * sIn, int sInLen,
    char * sOut, int sOutCap)
{
    snprintf(sOut, sOutCap, "{}");

    int koff, klen, voff, vlen, vtype, off;

    struct Key {
        const char * ptr;
        int len;
    };

    struct Key keys[JSON_MAX_INDICES];
    int numKeys = 0;

    for (off = 0; (off = mjson_next(sIn, sInLen, off, &koff, &klen, &voff, &vlen, &vtype)) != 0; ) {
        keys[numKeys].ptr = sIn + koff;
        keys[numKeys].len = klen;
        numKeys++;
    }

    for (int i = 0; i < numKeys; i++) {
        for (int j = i; j < numKeys; j++) {
            if (
                strncmp(
                    keys[i].ptr,
                    keys[j].ptr,
                    MIN(keys[i].len, keys[j].len)
                ) > 0
            ) {
                struct Key k = keys[i];
                keys[i] = keys[j];
                keys[j] = k;
            }
        }
    }

    for (int i = 0; i < numKeys; i++) {
        char jp[JSON_QUERY_SIZE];
        snprintf(jp, JSON_QUERY_SIZE, "$.%.*s", keys[i].len-2, keys[i].ptr+1);

        const char * valPtr;
        int valLen;
        mjson_find(sIn, sInLen, jp, &valPtr, &valLen);
        
        STATIC char newEntry[JSON_MAX_ENTRY_SIZE];
        snprintf(newEntry, JSON_MAX_ENTRY_SIZE, "{%.*s:%.*s}", keys[i].len, keys[i].ptr, valLen, valPtr);

        char * buffer = strdup(sOut);

        struct mjson_fixedbuf fb = { sOut, sOutCap, 0 };
        mjson_merge(buffer, strlen(buffer), newEntry, strlen(newEntry), mjson_print_fixed_buf, &fb);

        free(buffer);
    }

    // TODO: recursively sort entries

    return true;
}

bool JsonSign(
    MatrixClient * client,
    const char * sIn, int sInLen,
    char * sOut, int sOutCap)
{
    STATIC char signature[OLM_SIGNATURE_SIZE];
    size_t res =
        olm_account_sign(client->olmAccount.account,
            sIn, sInLen,
            signature, OLM_SIGNATURE_SIZE);
    
    int signatureLen = res;
    
    STATIC char thisSigningKey[SIGNING_KEY_SIZE];
    MatrixOlmAccountGetSigningKey(&client->olmAccount, thisSigningKey, SIGNING_KEY_SIZE);

    STATIC char signatureJson[JSON_SIGNATURE_SIZE];
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

    STATIC uint8_t random[OLM_ACCOUNT_RANDOM_SIZE];
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
    STATIC char deviceKeysJson[OLM_IDENTITY_KEYS_JSON_SIZE];
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
    STATIC char deviceKeysJson[OLM_IDENTITY_KEYS_JSON_SIZE];
    size_t res =
        olm_account_identity_keys(account->account,
            deviceKeysJson, OLM_IDENTITY_KEYS_JSON_SIZE);
    mjson_get_string(deviceKeysJson, res,
        "$.ed25519",
        key, keyCap);
    return true;
}

bool
MatrixOlmSessionFrom(
    MatrixOlmSession * session,
    OlmAccount * olmAccount,
    const char * deviceId,
    const char * deviceKey,
    const char * encrypted)
{
    memset(session, 0, sizeof(MatrixOlmSession));

    session->deviceId = deviceId;

    session->session =
        olm_session(session->memory);
    
    char * encryptedCopy = strdup(encrypted);
    
    size_t res =
        olm_create_inbound_session_from(session->session, olmAccount,
            deviceKey, strlen(deviceKey),
            encryptedCopy, strlen(encryptedCopy));
    
    if (res == olm_error()) {
        printf("error olm:%s\n", olm_session_last_error(session->session));
    }

    return res != olm_error();
}

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

    STATIC uint8_t random[OLM_OUTBOUND_SESSION_RANDOM_SIZE];
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

    return res != olm_error();
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
    STATIC uint8_t random[OLM_ENCRYPT_RANDOM_SIZE];
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
    STATIC uint8_t random[OLM_ENCRYPT_RANDOM_SIZE];
    Randomize(random, OLM_ENCRYPT_RANDOM_SIZE);

    size_t res =
        olm_decrypt(session->session,
            messageType,
            encrypted, strlen(encrypted),
            outBuffer, outBufferCap);
    
    if (res != olm_error() && res < outBufferCap)
        outBuffer[res] = '\0';

    return res != olm_error();
}

bool
MatrixMegolmInSessionInit(
    MatrixMegolmInSession * session,
    const char * roomId,
    const char * sessionId,
    const char * sessionKey, int sessionKeyLen)
{
    memset(session, 0, sizeof(MatrixMegolmInSession));
    
    strncpy(session->roomId, roomId, sizeof(session->roomId));
    strncpy(session->id, sessionId, sizeof(session->id));
    strncpy(session->key, sessionKey, sizeof(session->key));

    session->session =
        olm_inbound_group_session(session->memory);

    size_t res =
        olm_init_inbound_group_session(
        // olm_import_inbound_group_session(
            session->session,
            (const uint8_t *)sessionKey, sessionKeyLen);
    if (res == olm_error()) {
        printf("Error initializing Megolm session: %s\n", olm_inbound_group_session_last_error(session->session));
    }

    return res != olm_error();
}

bool
MatrixMegolmInSessionDecrypt(
    MatrixMegolmInSession * session,
    const char * encrypted, int encryptedLen,
    char * outDecrypted, int outDecryptedCap)
{
    // uint8_t buffer[1024];
    // memcpy(buffer, encrypted, encryptedLen);

    uint32_t megolmInMessageIndex;

    size_t res =
        olm_group_decrypt(session->session,
            (uint8_t *)encrypted, encryptedLen,
            (uint8_t *)outDecrypted, outDecryptedCap,
            &megolmInMessageIndex);
    
    printf("message index: %d\n", (int)megolmInMessageIndex);
    
    if (res == olm_error()) {
        printf("error decrypting megolm message: %s\n", olm_inbound_group_session_last_error(session->session));
    }
    else {
        printf("decrypted len: %d\n", res);
    }
    
    return true;
}

// https://matrix.org/docs/guides/end-to-end-encryption-implementation-guide#starting-a-megolm-session
bool
MatrixMegolmOutSessionInit(
    MatrixMegolmOutSession * session,
    const char * roomId)
{
    memset(session, 0, sizeof(MatrixMegolmOutSession));

    STATIC uint8_t random[MEGOLM_INIT_RANDOM_SIZE];
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
    MatrixClient * client)
{
    memset(client, 0, sizeof(MatrixClient));

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
    STATIC uint8_t random[OLM_ONETIME_KEYS_RANDOM_SIZE];
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
    mjson_snprintf(g_KeysUploadRequestBuffer, KEYS_UPLOAD_REQUEST_SIZE,
        "{");

    STATIC char onetimeKeysBuffer[1024];
    olm_account_one_time_keys(client->olmAccount.account,
        onetimeKeysBuffer, 1024);

    const char *keys;
    int keysLen;
    mjson_find(onetimeKeysBuffer, strlen(onetimeKeysBuffer), "$.curve25519", &keys, &keysLen);

    int koff, klen, voff, vlen, vtype, off = 0;
    while ((off = mjson_next(keys, keysLen, off, &koff, &klen, &voff, &vlen, &vtype)) != 0) {
        STATIC char keyJson[JSON_ONETIME_KEY_SIZE];
        
        int keyJsonLen =
            snprintf(keyJson, JSON_ONETIME_KEY_SIZE,
                "{\"key\":\"%.*s\"}",
                vlen-2, keys + voff+1);

        STATIC char keyJsonSigned[JSON_ONETIME_KEY_SIGNED_SIZE];

        JsonSign(client,
            keyJson, keyJsonLen,
            keyJsonSigned, JSON_ONETIME_KEY_SIGNED_SIZE);
        
        mjson_snprintf(g_KeysUploadRequestBuffer+strlen(g_KeysUploadRequestBuffer), KEYS_UPLOAD_REQUEST_SIZE-strlen(g_KeysUploadRequestBuffer),
            "\"signed_curve25519:%.*s\":%s,",
            klen-2, keys + koff+1,
            keyJsonSigned);
    }

    if (g_KeysUploadRequestBuffer[strlen(g_KeysUploadRequestBuffer)-1] == ',')
        g_KeysUploadRequestBuffer[strlen(g_KeysUploadRequestBuffer)-1] = '\0';

    mjson_snprintf(g_KeysUploadRequestBuffer+strlen(g_KeysUploadRequestBuffer), KEYS_UPLOAD_REQUEST_SIZE-strlen(g_KeysUploadRequestBuffer),
        "}");
        
    // STATIC char onetimeKeysSignedBuffer[KEYS_UPLOAD_REQUEST_SIGNED_SIZE];
    // JsonSign(client,
    //     g_KeysUploadRequestBuffer, strlen(g_KeysUploadRequestBuffer),
    //     onetimeKeysSignedBuffer, KEYS_UPLOAD_REQUEST_SIZE);
        
    // STATIC char finalEvent[KEYS_UPLOAD_REQUEST_SIGNED_SIZE];
    // snprintf(finalEvent, KEYS_UPLOAD_REQUEST_SIGNED_SIZE,
    // "{\"one_time_keys\":%s}", onetimeKeysSignedBuffer);
    snprintf(g_KeysUploadRequestSignedBuffer, KEYS_UPLOAD_REQUEST_SIGNED_SIZE,
    "{\"one_time_keys\":%s}", g_KeysUploadRequestBuffer);

    STATIC char responseBuffer[KEYS_UPLOAD_RESPONSE_SIZE];
    MatrixHttpPost(client->hc,
        KEYS_UPLOAD_URL,
        g_KeysUploadRequestSignedBuffer,
        responseBuffer, KEYS_UPLOAD_RESPONSE_SIZE,
        true);

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

    int deviceKeysBufferLen =
        mjson_snprintf(g_KeysUploadRequestBuffer, KEYS_UPLOAD_REQUEST_SIZE,
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

    JsonSign(client,
        g_KeysUploadRequestBuffer, deviceKeysBufferLen,
        g_KeysUploadRequestSignedBuffer, KEYS_UPLOAD_REQUEST_SIZE);
    
    STATIC char finalEvent[KEYS_UPLOAD_REQUEST_SIGNED_SIZE+30];
    snprintf(finalEvent, KEYS_UPLOAD_REQUEST_SIGNED_SIZE+30,
    "{\"device_keys\":%s}", g_KeysUploadRequestSignedBuffer);

    STATIC char responseBuffer[KEYS_UPLOAD_RESPONSE_SIZE];
    MatrixHttpPost(client->hc,
        KEYS_UPLOAD_URL,
        finalEvent,
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
    STATIC char requestBuffer[KEYS_CLAIM_REQUEST_SIZE];
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

    STATIC char responseBuffer[KEYS_CLAIM_RESPONSE_SIZE];
    MatrixHttpPost(client->hc,
        KEYS_CLAIM_URL,
        requestBuffer,
        responseBuffer, KEYS_CLAIM_RESPONSE_SIZE,
        true);
    
    STATIC char userIdEscaped[USER_ID_SIZE];
    JsonEscape(userId, strlen(userId),
        userIdEscaped, USER_ID_SIZE);
    
    STATIC char query[JSON_QUERY_SIZE];
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
    STATIC char requestBuffer[LOGIN_REQUEST_SIZE];

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
    
    STATIC char responseBuffer[LOGIN_RESPONSE_SIZE];
    bool result =
        MatrixHttpPost(client->hc,
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
        
    MatrixHttpSetAccessToken(client->hc, client->accessToken);

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
    STATIC char requestUrl[MAX_URL_LEN];
    sprintf(requestUrl,
        ROOM_SEND_URL, roomId, msgType, (int)time(NULL));

    STATIC char responseBuffer[ROOM_SEND_RESPONSE_SIZE];
    bool result =
        MatrixHttpPut(client->hc,
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
    STATIC char requestBuffer[ROOM_SEND_REQUEST_SIZE];
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
    if (! MatrixClientGetMegolmOutSession(client, roomId, &outSession))
        MatrixClientNewMegolmOutSession(client, roomId, &outSession);
        
    // encrypt
    MatrixMegolmOutSessionEncrypt(outSession,
        requestBuffer,
        g_EncryptedRequestBuffer, ENCRYPTED_REQUEST_SIZE);

    char thisDeviceKey[DEVICE_KEY_SIZE];
    MatrixOlmAccountGetDeviceKey(&client->olmAccount, thisDeviceKey, DEVICE_KEY_SIZE);
    

    // encrypted event json
    const char * senderKey = thisDeviceKey;
    const char * sessionId = outSession->id;
    const char * deviceId = client->deviceId;

    snprintf(g_EncryptedEventBuffer, ENCRYPTED_EVENT_SIZE,
        "{"
        "\"algorithm\":\"m.megolm.v1.aes-sha2\","
        "\"ciphertext\":\"%s\","
        "\"device_id\":\"%s\","
        "\"sender_key\":\"%s\","
        "\"session_id\":\"%s\""
        "}",
        g_EncryptedRequestBuffer,
        deviceId,
        senderKey,
        sessionId);

    // send
    return MatrixClientSendEvent(client,
        roomId,
        "m.room.encrypted",
        g_EncryptedEventBuffer);
}

// https://spec.matrix.org/v1.6/client-server-api/#get_matrixclientv3sync
bool
MatrixClientSync(
    MatrixClient * client,
    char * outSyncBuffer, int outSyncCap,
    const char * nextBatch)
{
    // filter={\"event_fields\":[\"to_device\"]}
    STATIC char url[MAX_URL_LEN];
    snprintf(url, MAX_URL_LEN,
        "/_matrix/client/v3/sync?timeout=%d" "%s" "%s",
        SYNC_TIMEOUT,
        "",
        // "&filter={\"event_fields\":[\"to_device\"]}",
        strlen(nextBatch) > 0 ? "&since=" : "");
    
    int index = strlen(url);

    for (size_t i = 0; i < strlen(nextBatch); i++) {
        char c = nextBatch[i];

        if (c == '~') {
            url[index++] = '%';
            url[index++] = '7';
            url[index++] = 'E';
        }
        else {
            url[index++] = c;
        }
    }
    url[index] = '\0';

    return
        MatrixHttpGet(client->hc,
            url,
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
    STATIC char url[MAX_URL_LEN];
    snprintf(url, MAX_URL_LEN,
        "/_matrix/client/v3/rooms/%s/event/%s",
            roomId,
            eventId);

    return
        MatrixHttpGet(client->hc,
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
    STATIC char eventBuffer[KEY_SHARE_EVENT_LEN];
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

    return false;
}

bool
MatrixClientNewMegolmOutSession(
    MatrixClient * client,
    const char * roomId,
    MatrixMegolmOutSession ** outSession)
{
    if (client->numMegolmOutSessions < NUM_MEGOLM_SESSIONS)
    {
        MatrixMegolmOutSession * result =
            &client->megolmOutSessions[client->numMegolmOutSessions];
        
        MatrixMegolmOutSessionInit(result,
            roomId);

        *outSession = result;

        client->numMegolmOutSessions++;

        return true;
    }

    return false;
}

bool
MatrixClientGetMegolmInSession(
    MatrixClient * client,
    const char * roomId, int roomIdLen,
    const char * sessionId, int sessionIdLen,
    MatrixMegolmInSession ** outSession)
{
    for (int i = 0; i < client->numMegolmInSessions; i++)
    {
        if (strncmp(client->megolmInSessions[i].roomId, roomId, roomIdLen) == 0 &&
            strncmp(client->megolmInSessions[i].id, sessionId, sessionIdLen) == 0)
        {
            *outSession = &client->megolmInSessions[i];
            return true;
        }
    }

    return false;
}

bool
MatrixClientNewMegolmInSession(
    MatrixClient * client,
    const char * roomId,
    const char * sessionId,
    const char * sessionKey,
    MatrixMegolmInSession ** outSession)
{
    if (client->numMegolmInSessions < NUM_MEGOLM_SESSIONS)
    {
        MatrixMegolmInSession * result =
            &client->megolmInSessions[client->numMegolmInSessions];
        
        MatrixMegolmInSessionInit(result,
            roomId,
            sessionId,
            sessionKey, strlen(sessionKey));
        
        *outSession = result;

        client->numMegolmInSessions++;

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
    const char * deviceId)
{
    // TODO: cancel requests
    MatrixClientSendDummy(client, userId, deviceId);

    STATIC char event[ROOMKEY_REQUEST_SIZE];
    snprintf(event, ROOMKEY_REQUEST_SIZE,
        "{"
            "\"action\":\"request\","
            "\"body\":{"
                "\"algorithm\":\"m.megolm.v1.aes-sha2\","
                "\"room_id\":\"%s\","
                "\"sender_key\":\"%s\","
                "\"session_id\":\"%s\""
            "},"
            "\"request_id\":\"%lld\","
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

    return false;
}

bool
MatrixClientNewOlmSessionIn(
    MatrixClient * client,
    const char * userId,
    const char * deviceId,
    const char * encrypted,
    MatrixOlmSession ** outSession)
{
    if (client->numOlmSessions < NUM_OLM_SESSIONS)
    {
        STATIC char deviceKey[DEVICE_KEY_SIZE];
        MatrixClientRequestDeviceKey(client,
            deviceId,
            deviceKey, DEVICE_KEY_SIZE);

        MatrixOlmSessionFrom(
            &client->olmSessions[client->numOlmSessions],
            client->olmAccount.account,
            deviceId,
            deviceKey,
            encrypted);

        *outSession = &client->olmSessions[client->numOlmSessions];
        
        client->numOlmSessions++;

        return true;
    }

    return false;
}

bool
MatrixClientNewOlmSessionOut(
    MatrixClient * client,
    const char * userId,
    const char * deviceId,
    MatrixOlmSession ** outSession)
{
    if (client->numOlmSessions < NUM_OLM_SESSIONS)
    {
        STATIC char deviceKey[DEVICE_KEY_SIZE];
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
    STATIC char requestUrl[MAX_URL_LEN];
    sprintf(requestUrl,
        TODEVICE_URL, msgType, (int)time(NULL));

    snprintf(g_TodeviceEventBuffer, TODEVICE_EVENT_SIZE,
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

    STATIC char responseBuffer[ROOM_SEND_RESPONSE_SIZE];
    bool result =
        MatrixHttpPut(client->hc,
            requestUrl,
            g_TodeviceEventBuffer,
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
    if (! MatrixClientGetOlmSession(client, userId, deviceId, &olmSession))
        MatrixClientNewOlmSessionOut(client, userId, deviceId, &olmSession);

    // create event json
    char targetDeviceKey[DEVICE_KEY_SIZE];
    MatrixClientRequestDeviceKey(client, deviceId, targetDeviceKey, DEVICE_KEY_SIZE);
    char targetSigningKey[SIGNING_KEY_SIZE];
    MatrixClientRequestSigningKey(client, deviceId, targetSigningKey, SIGNING_KEY_SIZE);
    
    char thisSigningKey[DEVICE_KEY_SIZE];
    MatrixOlmAccountGetSigningKey(&client->olmAccount, thisSigningKey, DEVICE_KEY_SIZE);

    snprintf(g_TodeviceEventBuffer, TODEVICE_EVENT_SIZE,
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

    // encrypt
    MatrixOlmSessionEncrypt(olmSession,
        g_TodeviceEventBuffer,
        g_EncryptedRequestBuffer, ENCRYPTED_REQUEST_SIZE);

    char thisDeviceKey[DEVICE_KEY_SIZE];
    MatrixOlmAccountGetDeviceKey(&client->olmAccount, thisDeviceKey, DEVICE_KEY_SIZE);

    snprintf(g_EncryptedEventBuffer, ENCRYPTED_EVENT_SIZE,
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
        g_EncryptedRequestBuffer,
        olm_session_has_received_message(olmSession->session),
        client->deviceId,
        thisDeviceKey);

    // send
    return MatrixClientSendToDevice(
        client,
        userId,
        deviceId,
        g_EncryptedEventBuffer,
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
    if (client->numDevices >= NUM_DEVICES) {
        printf("Maximum number of devices reached\n");
        return false;
    }

    STATIC char userIdEscaped[USER_ID_SIZE];
    JsonEscape(client->userId, strlen(client->userId),
        userIdEscaped, USER_ID_SIZE);

    STATIC char request[KEYS_QUERY_REQUEST_SIZE];
    snprintf(request, KEYS_QUERY_REQUEST_SIZE,
        "{\"device_keys\":{\"%s\":[]}}", client->userId);

    STATIC char responseBuffer[KEYS_QUERY_RESPONSE_SIZE];
    bool requestResult = MatrixHttpPost(client->hc,
        KEYS_QUERY_URL,
        request,
        responseBuffer, KEYS_QUERY_RESPONSE_SIZE,
        true);

    if (! requestResult)
        return false;

    // query for retrieving device keys for user id
    STATIC char query[JSON_QUERY_SIZE];
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
        STATIC char deviceKeyQuery[JSON_QUERY_SIZE];
        snprintf(deviceKeyQuery, JSON_QUERY_SIZE,
            "$.keys.curve25519:%s", d.deviceId);
        mjson_get_string(val, vlen,
            deviceKeyQuery, d.deviceKey, DEVICE_KEY_SIZE);

        // look for signing key in value
        STATIC char signingKeyQuery[JSON_QUERY_SIZE];
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
    STATIC char deleteRequest[1024];
    snprintf(deleteRequest, 1024,
        "{\"devices\":[\"%s\"]}",
        client->deviceId);
    STATIC char deleteResponse[1024];
    bool res = MatrixHttpPost(client->hc, "/_matrix/client/v3/delete_devices",
        deleteRequest, deleteResponse, 1024, true);
    return res;
}