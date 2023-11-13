#include <matrix.h>
#include <stdio.h>

#include <olm/sas.h>

#define SERVER       "https://matrix.org"
#define USER_ID      "@pscho:matrix.org"
#define DEVICE_ID    "ULZZOKJBYN"
#define SENDER_KEY   "LvVdoHsRRWNxRmG2GO2vky6o6S8RzADpPAaELsd1rjU"
#define ROOM_ID      "!XKFUjAsGrSSrpDFIxB:matrix.org"
#define EVENT_ID     "$_-y42DkC3OmJ_s40gYko7jMwrUQhoXfEut2pMV3E2J8"
#define SESSION_ID   "tzdnJbDrm82D/RpgkZKpILTifQ5Rads+tVzp3ax8+Ls"

void
GetLine(char * buffer, int n) {
    int c;
    int len = 0;

    while ((c = getchar()) != '\n' && len < n-1)
        buffer[len++] = c;
    
    buffer[len] = '\0';
}

int
main(void)
{
    MatrixClient client;
    MatrixClientInit(&client);
    
    MatrixHttpInit(&client.hc, SERVER);

    MatrixClientSetUserId(&client, USER_ID);

    MatrixClientLoginPassword(&client,
        "pscho",
        "Wc23EbmB9G3faMq",
        "Test1");

    printf("deviceId: %s\n", client.deviceId);

    MatrixClientGenerateOnetimeKeys(&client, 10);
    MatrixClientUploadOnetimeKeys(&client);
    MatrixClientUploadDeviceKeys(&client);

    static char eventBuffer[1024];
    MatrixClientGetRoomEvent(&client,
        ROOM_ID,
        EVENT_ID,
        eventBuffer, 1024);
    
    printf("event: %s\n", eventBuffer);

    // verify
    // char theirDeviceKey[DEVICE_KEY_SIZE];
    // MatrixClientRequestDeviceKey(&client,
    //     DEVICE_ID,
    //     theirDeviceKey, DEVICE_KEY_SIZE);
    
    char transactionId[256];
    GetLine(transactionId, 128);

    char verificationReadyBuffer[2048];
    snprintf(verificationReadyBuffer, 2048,
        "{"
        "\"from_device\":\"%s\","
        "\"methods\":[\"m.sas.v1\"],"
        "\"transaction_id\":\"%s\""
        "}",
        client.deviceId,
        transactionId);
    
    MatrixClientSendToDevice(&client,
        USER_ID,
        DEVICE_ID,
        verificationReadyBuffer,
        "m.key.verification.ready");
    
    OlmSAS * olmSas = olm_sas(malloc(olm_sas_size()));
    void * sasRandomBytes = malloc(olm_create_sas_random_length(olmSas));
    olm_create_sas(olmSas,
        sasRandomBytes,
        olm_create_sas_random_length(olmSas));
    
    OlmUtility * olmUtil = olm_utility(malloc(olm_utility_size()));
    
    char publicKey[128];
    char keyStartJson[1024];
    char concat[1024];
    char commitment[256];
    olm_sas_get_pubkey(olmSas,
        publicKey,
        128);
    GetLine(keyStartJson, 1024);
    printf("keyStartJson: %s\n", keyStartJson);
    snprintf(concat, 1024, "%s%s", publicKey, keyStartJson);
    printf("concat: %s\n", concat);
    olm_sha256(olmUtil, concat, strlen(concat), commitment, 256);
    printf("hash: %s\n", commitment);
    
    char verificationAcceptBuffer[2048];
    snprintf(verificationAcceptBuffer, 2048,
        "{"
        "\"commitment\":\"%s\","
        "\"hash\":\"sha256\","
        "\"key_agreement_protocol\":\"curve25519\","
        "\"message_authentication_code\":\"hkdf-hmac-sha256.v2\","
        "\"method\":\"m.sas.v1\","
        "\"short_authentication_string\":[\"decimal\"],"
        "\"transaction_id\":\"%s\""
        "}",
        commitment,
        transactionId);
    
    MatrixClientSendToDevice(&client,
        USER_ID,
        DEVICE_ID,
        verificationAcceptBuffer,
        "m.key.verification.accept");
    
    char theirPublicKey[128];
    GetLine(theirPublicKey, 128);
    olm_sas_set_their_key(olmSas, theirPublicKey, strlen(theirPublicKey));
    
    char verificationKeyBuffer[2048];
    snprintf(verificationKeyBuffer, 2048,
        "{"
        "\"key\":\"%s\","
        "\"transaction_id\":\"%s\""
        "}",
        publicKey,
        transactionId);
    
    MatrixClientSendToDevice(&client,
        USER_ID,
        DEVICE_ID,
        verificationKeyBuffer,
        "m.key.verification.key");

    char hkdfInfo[1024];
    int hkdfInfoLen =
        snprintf(hkdfInfo, 1024,
            "MATRIX_KEY_VERIFICATION_SAS%s%s%s%s%s",
            USER_ID,
            DEVICE_ID,
            USER_ID,
            client.deviceId,
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
    
    // mac
    const char * masterKey = "vt8tJ5/SxqkvXS+XoGxr+4rJNe8fJfZT3/e/FTwlFsI";

    char keyList[1024];
    char keyListMac[1024];
    char key1Id[1024];
    char key1[1024];
    char key1Mac[1024];
    char key2Id[1024];
    char key2[1024];
    char key2Mac[1024];

    if (strcmp(masterKey, client.deviceId) < 0) {
        //strcpy(key1Id, masterKey);
        snprintf(key1Id, 1024, "ed25519:%s", masterKey);
        strcpy(key1, masterKey);
        //strcpy(key2Id, client.deviceId);
        snprintf(key2Id, 1024, "ed25519:%s", client.deviceId);
        MatrixOlmAccountGetSigningKey(&client.olmAccount, key2, 1024);
    }
    else {
        //strcpy(key1Id, client.deviceId);
        snprintf(key1Id, 1024, "ed25519:%s", client.deviceId);
        MatrixOlmAccountGetSigningKey(&client.olmAccount, key1, 1024);
        //strcpy(key2Id, masterKey);
        snprintf(key2Id, 1024, "ed25519:%s", masterKey);
        strcpy(key2, masterKey);
    }

    snprintf(keyList, 1024,
        "%s,%s", key1Id, key2Id);
    
    char macInfo[1024];
    int macInfoLen;
    {
        macInfoLen =
            snprintf(macInfo, 1024,
                "MATRIX_KEY_VERIFICATION_MAC%s%s%s%s%s%s",
                USER_ID,
                client.deviceId,
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
                client.deviceId,
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
                client.deviceId,
                USER_ID,
                DEVICE_ID,
                transactionId,
                key2Id);
        olm_sas_calculate_mac_fixed_base64(olmSas, key2, strlen(key2), macInfo, macInfoLen, key2Mac, 1024);
    }

    printf("send mac:");
    getchar();

    char verificationMacBuffer[2048];
    snprintf(verificationMacBuffer, 2048,
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
    
    MatrixClientSendToDevice(&client,
        USER_ID,
        DEVICE_ID,
        verificationMacBuffer,
        "m.key.verification.mac");

    printf("send done:");
    getchar();

    char verificationDoneBuffer[2048];
    snprintf(verificationDoneBuffer, 2048,
        "{"
        "\"transaction_id\":\"%s\""
        "}",
        transactionId);
    
    MatrixClientSendToDevice(&client,
        USER_ID,
        DEVICE_ID,
        verificationDoneBuffer,
        "m.key.verification.done");
    
    // done

    // request room key

    getchar();
    
    MatrixClientRequestMegolmInSession(&client,
        ROOM_ID,
        SESSION_ID,
        SENDER_KEY,
        USER_ID,
        DEVICE_ID);

    // // decrypt room key

    MatrixOlmSession * olmSession;
    MatrixClientGetOlmSession(&client,
        USER_ID,
        DEVICE_ID,
        &olmSession);
    static char encrypted[2048];
    static char decrypted[2048];
    printf("encrypted:");
    fgets(encrypted, 2048, stdin);
    printf("(%d) %s;\n", strlen(encrypted), encrypted);
    MatrixOlmSessionDecrypt(olmSession,
        1, encrypted, decrypted, 2048);
    printf("decrypted: %s\n", decrypted);

    // int c;
    // while ((c = getchar()) != 'q') {
    //     printf("c: %c (%d)\n", c, c);
    //     static char syncBuffer[40000];
    //     MatrixClientSync(&client,
    //         syncBuffer, 40000);
    //     printf("sync: %s", syncBuffer);
    // }


    // static char decryptedBuffer[1024];
    // MatrixMegolmInSessionDecrypt(&megolmSession,
    //     eventBuffer,
    //     decryptedBuffer, 1024);

    // printf("%s\n", decryptedBuffer);

    getchar();

    MatrixClientDeleteDevice(&client);
        
    MatrixHttpDeinit(&client.hc);

    return 0;
}
