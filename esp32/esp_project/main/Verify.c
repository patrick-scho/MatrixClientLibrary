#include <matrix.h>
#include <mjson.h>
#include <olm/sas.h>

#include <stdio.h>

#define SERVER       "https://matrix.org"
#define USER_ID      "@pscho:matrix.org"

#define DEVICE_ID    "ULZZOKJBYN"
#define SENDER_KEY   "cjP41XzRlY+pd8DoiBuKQJj9o15mrx6gkrpqTkAPZ2c"
#define ROOM_ID      "!XKFUjAsGrSSrpDFIxB:matrix.org"
#define EVENT_ID     "$vOS09eUaI0CduqAcaIU5ZVk6ljLQfLspz7UThP8vaUM"
#define SESSION_ID   "90UbGLue3ADVhvW7hFjoA2c6yg0JJKs/lPdMDZXnZAk"

// main stack size: 3584

bool verified = false;
char transactionId[64];
OlmSAS * olmSas = NULL;

#define STATIC static

STATIC char encrypted[2048];
STATIC char decrypted[2048];

void
HandleEvent(
    MatrixClient * client,
    const char * event, int eventLen
) {
    STATIC char eventType[128];
    memset(eventType, 0, sizeof(eventType));
    mjson_get_string(event, eventLen, "$.type", eventType, 128);

    if (strcmp(eventType, "m.key.verification.request") == 0) {
        mjson_get_string(event, eventLen, "$.content.transaction_id", transactionId, 256);
        
        char verificationReadyBuffer[2048];
        snprintf(verificationReadyBuffer, 2048,
            "{"
            "\"from_device\":\"%s\","
            "\"methods\":[\"m.sas.v1\"],"
            "\"transaction_id\":\"%s\""
            "}",
            client->deviceId,
            transactionId);
        
        MatrixClientSendToDevice(client,
            USER_ID,
            DEVICE_ID,
            verificationReadyBuffer,
            "m.key.verification.ready");
    }
    else if (strcmp(eventType, "m.key.verification.start") == 0) {
        olmSas = olm_sas(malloc(olm_sas_size()));
        void * sasRandomBytes = malloc(olm_create_sas_random_length(olmSas));
        olm_create_sas(olmSas,
            sasRandomBytes,
            olm_create_sas_random_length(olmSas));
        
        OlmUtility * olmUtil = olm_utility(malloc(olm_utility_size()));
        
        STATIC char publicKey[64];
        STATIC char keyStartJsonCanonical[512];
        STATIC char concat[512+64];
        STATIC char commitment[1024];
        olm_sas_get_pubkey(olmSas,
            publicKey,
            64);
        printf("public key: %.*s\n", olm_sas_pubkey_length(olmSas), publicKey);

        const char * keyStartJson;
        int keyStartJsonLen;
        mjson_find(event, eventLen, "$.content", &keyStartJson, &keyStartJsonLen);
        JsonCanonicalize(keyStartJson, keyStartJsonLen, keyStartJsonCanonical, 512);

        printf("json:\n%.*s\ncanonical json:\n%s\n", keyStartJsonLen, keyStartJson, keyStartJsonCanonical);

        int concatLen =
            snprintf(concat, 512+64, "%.*s%s", olm_sas_pubkey_length(olmSas), publicKey, keyStartJsonCanonical);

        int commitmentLen =
            olm_sha256(olmUtil, concat, concatLen, commitment, 1024);
        
        STATIC char verificationAcceptBuffer[512];
        snprintf(verificationAcceptBuffer, 512,
            "{"
            "\"commitment\":\"%.*s\","
            "\"hash\":\"sha256\","
            "\"key_agreement_protocol\":\"curve25519\","
            "\"message_authentication_code\":\"hkdf-hmac-sha256.v2\","
            "\"method\":\"m.sas.v1\","
            "\"short_authentication_string\":[\"decimal\"],"
            "\"transaction_id\":\"%s\""
            "}",
            commitmentLen, commitment,
            transactionId);
        
        MatrixClientSendToDevice(client,
            USER_ID,
            DEVICE_ID,
            verificationAcceptBuffer,
            "m.key.verification.accept");
    }
    else if (strcmp(eventType, "m.key.verification.key") == 0) {
        STATIC char publicKey[128];
        olm_sas_get_pubkey(olmSas,
            publicKey,
            128);

        STATIC char theirPublicKey[128];
        int theirPublicKeyLen =
            mjson_get_string(event, eventLen, "$.content.key", theirPublicKey, 128);
        
        printf("event: %.*s\n", eventLen, event);
        printf("theirPublicKey: %.*s\n", theirPublicKeyLen, theirPublicKey);
        printf("publicKey: %.*s\n", olm_sas_pubkey_length(olmSas), publicKey);

        olm_sas_set_their_key(olmSas, theirPublicKey, theirPublicKeyLen);
        
        STATIC char verificationKeyBuffer[256];
        snprintf(verificationKeyBuffer, 256,
            "{"
            "\"key\":\"%.*s\","
            "\"transaction_id\":\"%s\""
            "}",
            olm_sas_pubkey_length(olmSas), publicKey,
            transactionId);
        
        MatrixClientSendToDevice(client,
            USER_ID,
            DEVICE_ID,
            verificationKeyBuffer,
            "m.key.verification.key");
        
        // sas
        STATIC char hkdfInfo[1024];
        int hkdfInfoLen =
            snprintf(hkdfInfo, 1024,
                "MATRIX_KEY_VERIFICATION_SAS%s%s%s%s%s",
                USER_ID,
                DEVICE_ID,
                USER_ID,
                client->deviceId,
                transactionId);

        unsigned char sasBytes[5];
        olm_sas_generate_bytes(olmSas,
            hkdfInfo, hkdfInfoLen,
            sasBytes, 5);
        int b0 = sasBytes[0];
        int b1 = sasBytes[1];
        int b2 = sasBytes[2];
        int b3 = sasBytes[3];
        int b4 = sasBytes[4];
        
        printf("%d %d %d %d %d\n", b0, b1, b2, b3, b4);

        // https://spec.matrix.org/v1.7/client-server-api/#sas-method-decimal
        printf("%d | %d | %d\n",
            (b0 << 5 | b1 >> 3) + 1000,
            ((b1 & 0x7) << 10 | b2 << 2 | b3 >> 6) + 1000,
            ((b3 & 0x3F) << 7 | b4 >> 1) + 1000);
        printf("%d | %d | %d\n",
            ((b0 << 5) | (b1 >> 3)) + 1000,
            (((b1 & 0x7) << 10) | (b2 << 2) | (b3 >> 6)) + 1000,
            (((b3 & 0x3F) << 7) | (b4 >> 1)) + 1000);
    }
    else if (strcmp(eventType, "m.key.verification.mac") == 0) {        
        // mac
        const char * masterKey = "vt8tJ5/SxqkvXS+XoGxr+4rJNe8fJfZT3/e/FTwlFsI";

        STATIC char keyList[256];
        STATIC char keyListMac[256];
        STATIC char key1Id[128];
        STATIC char key1[128];
        STATIC char key1Mac[128];
        STATIC char key2Id[128];
        STATIC char key2[128];
        STATIC char key2Mac[128];

        if (strcmp(masterKey, client->deviceId) < 0) {
            snprintf(key1Id, 1024, "ed25519:%s", masterKey);
            strcpy(key1, masterKey);
            snprintf(key2Id, 1024, "ed25519:%s", client->deviceId);
            MatrixOlmAccountGetSigningKey(&client->olmAccount, key2, 1024);
        }
        else {
            snprintf(key1Id, 1024, "ed25519:%s", client->deviceId);
            MatrixOlmAccountGetSigningKey(&client->olmAccount, key1, 1024);
            snprintf(key2Id, 1024, "ed25519:%s", masterKey);
            strcpy(key2, masterKey);
        }

        snprintf(keyList, 1024,
            "%s,%s", key1Id, key2Id);
        
        STATIC char macInfo[1024];
        int macInfoLen;
        {
            macInfoLen =
                snprintf(macInfo, 1024,
                    "MATRIX_KEY_VERIFICATION_MAC%s%s%s%s%s%s",
                    USER_ID,
                    client->deviceId,
                    USER_ID,
                    DEVICE_ID,
                    transactionId,
                    "KEY_IDS");
            olm_sas_calculate_mac_fixed_base64(olmSas, keyList, strlen(keyList), macInfo, macInfoLen, keyListMac, 1024);
        }
        {
            macInfoLen =
                snprintf(macInfo, 1024,
                    "MATRIX_KEY_VERIFICATION_MAC%s%s%s%s%s%s",
                    USER_ID,
                    client->deviceId,
                    USER_ID,
                    DEVICE_ID,
                    transactionId,
                    key1Id);
            olm_sas_calculate_mac_fixed_base64(olmSas, key1, strlen(key1), macInfo, macInfoLen, key1Mac, 1024);
        }
        {
            macInfoLen =
                snprintf(macInfo, 1024,
                    "MATRIX_KEY_VERIFICATION_MAC%s%s%s%s%s%s",
                    USER_ID,
                    client->deviceId,
                    USER_ID,
                    DEVICE_ID,
                    transactionId,
                    key2Id);
            olm_sas_calculate_mac_fixed_base64(olmSas, key2, strlen(key2), macInfo, macInfoLen, key2Mac, 1024);
        }

        STATIC char verificationMacBuffer[1024];
        snprintf(verificationMacBuffer, 1024,
            "{"
            "\"keys\":\"%s\","
            "\"mac\":{"
            "\"%s\":\"%s\","
            "\"%s\":\"%s\""
            "},"
            "\"transaction_id\":\"%s\""
            "}",
            keyListMac,
            key1Id,
            key1Mac,
            key2Id,
            key2Mac,
            transactionId);
        
        MatrixClientSendToDevice(client,
            USER_ID,
            DEVICE_ID,
            verificationMacBuffer,
            "m.key.verification.mac");

        STATIC char verificationDoneBuffer[128];
        snprintf(verificationDoneBuffer, 128,
            "{"
            "\"transaction_id\":\"%s\""
            "}",
            transactionId);
        
        MatrixClientSendToDevice(client,
            USER_ID,
            DEVICE_ID,
            verificationDoneBuffer,
            "m.key.verification.done");
        
        verified = true;
    }
    else if (strcmp(eventType, "m.room.encrypted") == 0) {
        STATIC char algorithm[128];
        mjson_get_string(event, eventLen, "$.content.algorithm", algorithm, 128);

        if (strcmp(algorithm, "m.olm.v1.curve25519-aes-sha2") == 0) {
            STATIC char thisDeviceKey[DEVICE_KEY_SIZE];
            MatrixOlmAccountGetDeviceKey(&client->olmAccount, thisDeviceKey, DEVICE_KEY_SIZE);

            STATIC char jp[128];
            snprintf(jp, 128, "$.content.ciphertext.%s.type", thisDeviceKey);

            double messageType;
            mjson_get_number(event, eventLen, jp, &messageType);
            int messageTypeInt = (int)messageType;

            snprintf(jp, 128, "$.content.ciphertext.%s.body", thisDeviceKey);

            mjson_get_string(event, eventLen, jp, encrypted, 2048);

            MatrixOlmSession * olmSession;
            
            if (! MatrixClientGetOlmSession(client, USER_ID, DEVICE_ID, &olmSession))
            {
                if (messageTypeInt == 0) {
                    MatrixClientNewOlmSessionIn(client,
                        USER_ID,
                        DEVICE_ID,
                        encrypted,
                        &olmSession);
                }
                else {
                    MatrixClientNewOlmSessionOut(client,
                        USER_ID,
                        DEVICE_ID,
                        &olmSession);
                }
            }

            printf("event: %.*s\n", eventLen, event);
            printf("encrypted: %s\n", encrypted);
            
            MatrixOlmSessionDecrypt(olmSession,
                messageTypeInt, encrypted, decrypted, 2048);
            
            printf("decrypted: %s\n", decrypted);
            
            HandleEvent(client, decrypted, strlen(decrypted));
        }
    }
    else if (strcmp(eventType, "m.room_key") == 0 ||
             strcmp(eventType, "m.forwarded_room_key") == 0) {
        STATIC char roomId[128];
        STATIC char sessionId[128];
        STATIC char sessionKey[1024];
        mjson_get_string(event, eventLen, "$.content.room_id", roomId, 128);
        mjson_get_string(event, eventLen, "$.content.session_id", sessionId, 128);
        mjson_get_string(event, eventLen, "$.content.session_key", sessionKey, 1024);
        
        printf("sessionId: %s\n", sessionId);
        printf("sessionKey: %s\n", sessionKey);

        MatrixMegolmInSession * megolmInSession;
        MatrixClientNewMegolmInSession(client, roomId, sessionId, sessionKey, &megolmInSession);
    }
}

void
HandleRoomEvent(
    MatrixClient * client,
    const char * room, int roomLen,
    const char * event, int eventLen)
{
    STATIC char eventType[128];
    memset(eventType, 0, sizeof(eventType));
    mjson_get_string(event, eventLen, "$.type", eventType, 128);

    if (strcmp(eventType, "m.room.encrypted") == 0) {
        STATIC char algorithm[128];
        mjson_get_string(event, eventLen, "$.content.algorithm", algorithm, 128);

        if (strcmp(algorithm, "m.megolm.v1.aes-sha2") == 0) {
            STATIC char sessionId[128];
            int sessionIdLen =
                mjson_get_string(event, eventLen, "$.content.session_id", sessionId, 128);

            bool res;

            MatrixMegolmInSession * megolmInSession;
            res = MatrixClientGetMegolmInSession(client,
                room, roomLen,
                sessionId, sessionIdLen,
                &megolmInSession);

            if (res) {
                mjson_get_string(event, eventLen, "$.content.ciphertext", encrypted, 2048);

                MatrixMegolmInSessionDecrypt(megolmInSession, encrypted, strlen(encrypted), decrypted, 2048);

                printf("decrypted: %s\n", decrypted);

                HandleEvent(client, decrypted, strlen(decrypted));
            }
            else {
                printf("megolm session not known\n");
            }
        }
    }
    HandleEvent(client, event, eventLen);
}

void
Sync(
    MatrixClient * client,
    char * syncBuffer, int syncBufferLen)
{
    STATIC char nextBatch[1024] = {0};

    MatrixClientSync(client, syncBuffer, syncBufferLen, nextBatch);
    
    int res;

    const char * s = syncBuffer;
    int slen = strlen(syncBuffer);
    
    printf("sync:\n\n%s\n\n", syncBuffer);
    
    // {
    // int koff, klen, voff, vlen, vtype, off = 0;
    // for (off = 0; (off = mjson_next(s, slen, off, &koff, &klen,
    //                                 &voff, &vlen, &vtype)) != 0; ) {
    //     const char * k = s + koff;
    //     const char * v = s + voff;

    //     printf("%.*s: %.100s\n", klen, k, v);
    // }
    // }

    mjson_get_string(s, slen, "$.next_batch", nextBatch, 1024);

    // to_device

    const char * events;
    int eventsLen;
    res =
        mjson_find(s, slen, "$.to_device.events", &events, &eventsLen);
    
    if (res != MJSON_TOK_INVALID) {
        {
        int koff, klen, voff, vlen, vtype, off = 0;
        for (off = 0; (off = mjson_next(events, eventsLen, off, &koff, &klen,
                                        &voff, &vlen, &vtype)) != 0; ) {
            const char * v = events + voff;

            HandleEvent(client, v, vlen);
        }
        }
    }

    // rooms
    
    const char * rooms;
    int roomsLen;
    res =
        mjson_find(s, slen, "$.rooms.join", &rooms, &roomsLen);
    
    if (res != MJSON_TOK_INVALID) {
        {
        int koff, klen, voff, vlen, vtype, off = 0;
        for (off = 0; (off = mjson_next(rooms, roomsLen, off, &koff, &klen,
                                        &voff, &vlen, &vtype)) != 0; ) {
            const char * k = rooms + koff;
            const char * v = rooms + voff;

            const char * events;
            int eventsLen;
            res =
                mjson_find(v, vlen, "$.timeline.events", &events, &eventsLen);
            
            if (res != MJSON_TOK_INVALID) {
                {
                int koff2, klen2, voff2, vlen2, vtype2, off2 = 0;
                for (off2 = 0; (off2 = mjson_next(events, eventsLen, off2, &koff2, &klen2,
                                                &voff2, &vlen2, &vtype2)) != 0; ) {
                    const char * v2 = events + voff2;

                    HandleRoomEvent(client,
                        k+1, klen-2,
                        v2, vlen2);
                }
                }
            }
        }
        }
    }
}


int
main(void)
{
    // sizeof(MatrixOlmAccount);
    // sizeof(MatrixMegolmInSession);
    // sizeof(MatrixMegolmOutSession);
    // sizeof(MatrixOlmSession);    
    // sizeof(MatrixDevice);

    // STATIC MatrixClient _client;
    // MatrixClient * client = &_client;
    MatrixClient * client = (MatrixClient*)malloc(sizeof(MatrixClient));
    MatrixClientInit(client);

    MatrixHttpInit(&client->hc, SERVER);
    MatrixClientSetUserId(client, USER_ID);

    MatrixClientLoginPassword(client,
        "pscho",
        "Wc23EbmB9G3faMq",
        "Test1");
    printf("deviceId: %s\n", client->deviceId);
    MatrixClientGenerateOnetimeKeys(client, 10);
    MatrixClientUploadOnetimeKeys(client);
    MatrixClientUploadDeviceKey(client);

    STATIC char eventBuffer[1024];
    MatrixClientGetRoomEvent(client,
        ROOM_ID,
        EVENT_ID,
        eventBuffer, 1024);
    printf("event: %s\n", eventBuffer);

    #define SYNC_BUFFER_SIZE 1024*10

    // char * syncBuffer = (char*)malloc(SYNC_BUFFER_SIZE);
    STATIC char syncBuffer[SYNC_BUFFER_SIZE];

    while (! verified) {
        Sync(client, syncBuffer, SYNC_BUFFER_SIZE);
    }

    printf("verified!\n");
    
    int c;
    while ((c=getchar()) != 'q') {
        printf("getchar() = %c [%d]\n", c, c);
        Sync(client, syncBuffer, SYNC_BUFFER_SIZE);
    }
    
    // MatrixClientRequestMegolmInSession(client,
    //     ROOM_ID,
    //     SESSION_ID,
    //     SENDER_KEY,
    //     USER_ID,
    //     DEVICE_ID);

    // MatrixMegolmInSession * megolmInSession;
    // while (! MatrixClientGetMegolmInSession(client,
    //     ROOM_ID, strlen(ROOM_ID),
    //     SESSION_ID, strlen(SESSION_ID),
    //     &megolmInSession))
    //     Sync(client, syncBuffer, SYNC_BUFFER_SIZE);

    // int encryptedLen =
    //     mjson_get_string(eventBuffer, strlen(eventBuffer), "$.content.ciphertext", encrypted, 1024);
    
    // printf("encrypted: [%.*s]\n", encryptedLen, encrypted);

    // MatrixMegolmInSessionDecrypt(megolmInSession,
    //     encrypted, encryptedLen,
    //     decrypted, 1024);

    // printf("decrypted: %s\n", decrypted);

    MatrixClientDeleteDevice(client);
        
    MatrixHttpDeinit(&client->hc);

    return 0;
}

#include "wifi.h"

void
app_main(void)
{
    wifi_init("Hundehuette", "Affensicherespw55");

    main();
}
